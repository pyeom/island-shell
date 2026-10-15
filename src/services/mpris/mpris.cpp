#include "mpris.h"

#include "dbusutil.h"

#include <QCoreApplication>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QJSEngine>

namespace island {

namespace {

constexpr auto kPath = "/org/mpris/MediaPlayer2";
constexpr auto kRoot = "org.mpris.MediaPlayer2";
constexpr auto kPlayer = "org.mpris.MediaPlayer2.Player";
constexpr auto kProps = "org.freedesktop.DBus.Properties";

using dbus::toMap;
using dbus::toStringList;
using dbus::unwrap;

} // namespace

// ---- MprisPlayer -----------------------------------------------------------

MprisPlayer::MprisPlayer(const QString& service, QObject* parent)
    : QObject(parent), m_service(service) {
    auto bus = QDBusConnection::sessionBus();
    bus.connect(service, QLatin1String(kPath), QLatin1String(kProps),
                QStringLiteral("PropertiesChanged"), this,
                SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));
    bus.connect(service, QLatin1String(kPath), QLatin1String(kPlayer), QStringLiteral("Seeked"),
                this, SLOT(onSeeked(qlonglong)));

    // Position is extrapolated locally; poll now and then to stay in sync.
    m_positionTick.setInterval(1000);
    connect(&m_positionTick, &QTimer::timeout, this, [this] {
        emit positionChanged();
        if (m_positionClock.isValid() && m_positionClock.elapsed() > 5000)
            fetchPosition();
    });
    fetchAll();
}

qreal MprisPlayer::position() const {
    double us = static_cast<double>(m_positionUs);
    if (playing() && m_positionClock.isValid())
        us += static_cast<double>(m_positionClock.elapsed()) * 1000.0 * m_rate;
    if (m_lengthUs > 0)
        us = std::min(us, static_cast<double>(m_lengthUs));
    return us / 1e6;
}

void MprisPlayer::call(const QString& method, const QVariantList& args) {
    auto msg = QDBusMessage::createMethodCall(m_service, QLatin1String(kPath),
                                              QLatin1String(kPlayer), method);
    msg.setArguments(args);
    QDBusConnection::sessionBus().asyncCall(msg);
}

void MprisPlayer::playPause() { call(QStringLiteral("PlayPause")); }
void MprisPlayer::next() { call(QStringLiteral("Next")); }
void MprisPlayer::previous() { call(QStringLiteral("Previous")); }

void MprisPlayer::raise() {
    auto msg = QDBusMessage::createMethodCall(m_service, QLatin1String(kPath),
                                              QLatin1String(kRoot), QStringLiteral("Raise"));
    QDBusConnection::sessionBus().asyncCall(msg);
}

void MprisPlayer::seekTo(qreal seconds) {
    if (!m_canSeek || m_trackId.isEmpty())
        return;
    const auto us = static_cast<qlonglong>(seconds * 1e6);
    call(QStringLiteral("SetPosition"),
         {QVariant::fromValue(QDBusObjectPath(m_trackId)), QVariant::fromValue(us)});
    m_positionUs = us;
    m_positionClock.restart();
    emit positionChanged();
}

void MprisPlayer::fetchAll() {
    auto bus = QDBusConnection::sessionBus();
    for (const auto* iface : {kRoot, kPlayer}) {
        auto msg = QDBusMessage::createMethodCall(m_service, QLatin1String(kPath),
                                                  QLatin1String(kProps), QStringLiteral("GetAll"));
        msg.setArguments({QLatin1String(iface)});
        auto* watcher = new QDBusPendingCallWatcher(bus.asyncCall(msg), this);
        const QString name = QLatin1String(iface);
        connect(watcher, &QDBusPendingCallWatcher::finished, this,
                [this, name](QDBusPendingCallWatcher* w) {
                    QDBusPendingReply<QVariantMap> reply = *w;
                    w->deleteLater();
                    if (reply.isError())
                        return;
                    onPropertiesChanged(name, reply.value(), {});
                });
    }
}

void MprisPlayer::fetchPosition() {
    auto msg = QDBusMessage::createMethodCall(m_service, QLatin1String(kPath),
                                              QLatin1String(kProps), QStringLiteral("Get"));
    msg.setArguments({QLatin1String(kPlayer), QStringLiteral("Position")});
    auto* watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
        QDBusPendingReply<QDBusVariant> reply = *w;
        w->deleteLater();
        m_positionClock.restart();
        if (reply.isError())
            return;
        m_positionUs = reply.value().variant().toLongLong();
        emit positionChanged();
    });
}

void MprisPlayer::onPropertiesChanged(const QString& interface, const QVariantMap& changedProps,
                                      const QStringList& /*invalidated*/) {
    if (interface == QLatin1String(kRoot)) {
        if (changedProps.contains(QStringLiteral("Identity")))
            m_identity = unwrap(changedProps.value(QStringLiteral("Identity"))).toString();
        if (changedProps.contains(QStringLiteral("DesktopEntry")))
            m_desktopEntry = unwrap(changedProps.value(QStringLiteral("DesktopEntry"))).toString();
        emit changed();
    } else if (interface == QLatin1String(kPlayer)) {
        applyPlayer(changedProps);
    }
}

void MprisPlayer::applyPlayer(const QVariantMap& props) {
    const bool wasPlaying = playing();
    auto get = [&](const char* key) { return unwrap(props.value(QLatin1String(key))); };
    if (props.contains(QStringLiteral("PlaybackStatus"))) {
        // Freeze the extrapolated position at the moment the state changes.
        m_positionUs = static_cast<qlonglong>(position() * 1e6);
        m_positionClock.restart();
        m_status = get("PlaybackStatus").toString();
    }
    if (props.contains(QStringLiteral("Metadata")))
        applyMetadata(toMap(props.value(QStringLiteral("Metadata"))));
    if (props.contains(QStringLiteral("Position"))) {
        m_positionUs = get("Position").toLongLong();
        m_positionClock.restart();
    }
    if (props.contains(QStringLiteral("Rate")))
        m_rate = get("Rate").toDouble();
    if (props.contains(QStringLiteral("CanGoNext")))
        m_canGoNext = get("CanGoNext").toBool();
    if (props.contains(QStringLiteral("CanGoPrevious")))
        m_canGoPrevious = get("CanGoPrevious").toBool();
    if (props.contains(QStringLiteral("CanSeek")))
        m_canSeek = get("CanSeek").toBool();
    if (props.contains(QStringLiteral("CanControl")))
        m_canControl = get("CanControl").toBool();

    if (playing())
        m_positionTick.start();
    else
        m_positionTick.stop();
    emit changed();
    emit positionChanged();
    if (playing() && !wasPlaying)
        emit startedPlaying();
}

void MprisPlayer::applyMetadata(const QVariantMap& metadata) {
    auto get = [&](const char* key) { return unwrap(metadata.value(QLatin1String(key))); };
    const auto trackId = get("mpris:trackid");
    m_trackId = trackId.canConvert<QDBusObjectPath>() ? trackId.value<QDBusObjectPath>().path()
                                                      : trackId.toString();
    m_title = get("xesam:title").toString();
    m_artist = toStringList(metadata.value(QStringLiteral("xesam:artist"))).join(u", ");
    m_album = get("xesam:album").toString();
    m_artUrl = get("mpris:artUrl").toString();
    m_lengthUs = get("mpris:length").toLongLong();
    // A new track restarts the clock; players send Position separately.
    fetchPosition();
}

void MprisPlayer::onSeeked(qlonglong positionUs) {
    m_positionUs = positionUs;
    m_positionClock.restart();
    emit positionChanged();
}

// ---- Mpris -----------------------------------------------------------------

Mpris* Mpris::instance() {
    static auto* mpris = new Mpris(QCoreApplication::instance());
    return mpris;
}

Mpris* Mpris::create(QQmlEngine*, QJSEngine*) {
    auto* mpris = instance();
    QJSEngine::setObjectOwnership(mpris, QJSEngine::CppOwnership);
    return mpris;
}

Mpris::Mpris(QObject* parent) : QObject(parent) {
    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return;
    bus.connect(QStringLiteral("org.freedesktop.DBus"), QStringLiteral("/org/freedesktop/DBus"),
                QStringLiteral("org.freedesktop.DBus"), QStringLiteral("NameOwnerChanged"), this,
                SLOT(onNameOwnerChanged(QString, QString, QString)));
    const auto names = bus.interface()->registeredServiceNames().value();
    for (const auto& name : names)
        if (name.startsWith(QLatin1String("org.mpris.MediaPlayer2.")))
            add(name);
}

QList<MprisPlayer*> Mpris::players() const {
    QList<MprisPlayer*> out;
    for (const auto& p : m_players)
        if (p)
            out.append(p);
    return out;
}

void Mpris::setCurrent(MprisPlayer* player) {
    if (m_current == player)
        return;
    m_current = player;
    emit currentChanged();
}

void Mpris::onNameOwnerChanged(const QString& name, const QString& oldOwner,
                               const QString& newOwner) {
    if (!name.startsWith(QLatin1String("org.mpris.MediaPlayer2.")))
        return;
    if (!oldOwner.isEmpty())
        remove(name);
    if (!newOwner.isEmpty())
        add(name);
}

void Mpris::add(const QString& service) {
    auto* player = new MprisPlayer(service, this);
    connect(player, &MprisPlayer::startedPlaying, this, [this, player] { setCurrent(player); });
    connect(player, &MprisPlayer::changed, this, &Mpris::pickCurrent);
    m_players.append(player);
    emit playersChanged();
    pickCurrent();
}

void Mpris::remove(const QString& service) {
    for (auto it = m_players.begin(); it != m_players.end(); ++it) {
        if (*it && (*it)->service() == service) {
            MprisPlayer* p = *it;
            m_players.erase(it);
            if (m_current == p) {
                m_current = nullptr;
                pickCurrent();
                emit currentChanged();
            }
            emit playersChanged();
            p->deleteLater();
            return;
        }
    }
}

void Mpris::pickCurrent() {
    if (m_current && m_current->status() != u"Stopped")
        return;
    for (const auto& p : m_players) {
        if (p && p->playing()) {
            setCurrent(p);
            return;
        }
    }
}

} // namespace island
