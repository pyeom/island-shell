#include "notifications.h"

#include "config.h"
#include "historyadaptor.h"
#include "icons.h"
#include "notificationrules.h"
#include "notificationsadaptor.h"
#include "shellcontrol.h"

#include <QCoreApplication>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMetaType>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QImage>
#include <QJSEngine>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>

Q_LOGGING_CATEGORY(lcNotif, "island.notifications")

using namespace Qt::StringLiterals;

namespace island {

// ---- NotificationModel -----------------------------------------------------

int NotificationModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant NotificationModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_items.size())
        return {};
    const auto map = toMap(m_items[index.row()]);
    const auto names = roleNames();
    return map.value(QString::fromLatin1(names.value(role)));
}

QHash<int, QByteArray> NotificationModel::roleNames() const {
    return {
        {IdRole, "id"},
        {AppNameRole, "appName"},
        {IconRole, "icon"},
        {SummaryRole, "summary"},
        {BodyRole, "body"},
        {ImageRole, "image"},
        {ActionsRole, "actions"},
        {UrgencyRole, "urgency"},
        {TimeRole, "time"},
        {HasReplyRole, "hasReply"},
        {ReplyPlaceholderRole, "replyPlaceholder"},
    };
}

QVariantMap NotificationModel::toMap(const NotificationData& n) {
    QVariantList actions;
    for (qsizetype i = 0; i + 1 < n.actions.size(); i += 2) {
        if (n.actions[i] == u"inline-reply")
            continue;
        actions.append(QVariantMap{{QStringLiteral("key"), n.actions[i]},
                                   {QStringLiteral("label"), n.actions[i + 1]}});
    }
    const auto iconName = !n.appIcon.isEmpty() ? n.appIcon : n.desktopEntry;
    return {
        {QStringLiteral("id"), n.id},
        {QStringLiteral("appName"), n.appName.isEmpty() ? QStringLiteral("Unknown") : n.appName},
        {QStringLiteral("icon"), Icons::source(iconName, QStringLiteral("dialog-information"))},
        {QStringLiteral("summary"), n.summary},
        {QStringLiteral("body"), n.body},
        {QStringLiteral("image"), n.image.isEmpty() ? QString() : Icons::source(n.image)},
        {QStringLiteral("actions"), actions},
        {QStringLiteral("hasDefault"), n.actions.contains(QStringLiteral("default"))},
        {QStringLiteral("urgency"), n.urgency},
        {QStringLiteral("time"), n.time},
        {QStringLiteral("hasReply"), n.inlineReply},
        {QStringLiteral("replyPlaceholder"), n.replyPlaceholder},
    };
}

int NotificationModel::indexOf(uint id) const {
    for (int i = 0; i < m_items.size(); ++i)
        if (m_items[i].id == id)
            return i;
    return -1;
}

const NotificationData* NotificationModel::find(uint id) const {
    const int i = indexOf(id);
    return i < 0 ? nullptr : &m_items[i];
}

void NotificationModel::upsert(const NotificationData& n) {
    const int i = indexOf(n.id);
    if (i == 0) {
        m_items[0] = n;
        emit dataChanged(index(0), index(0));
        return;
    }
    if (i > 0) {
        beginRemoveRows({}, i, i);
        m_items.removeAt(i);
        endRemoveRows();
    }
    beginInsertRows({}, 0, 0);
    m_items.prepend(n);
    endInsertRows();
    if (i < 0)
        emit countChanged();
}

bool NotificationModel::remove(uint id) {
    const int i = indexOf(id);
    if (i < 0)
        return false;
    beginRemoveRows({}, i, i);
    m_items.removeAt(i);
    endRemoveRows();
    emit countChanged();
    return true;
}

void NotificationModel::clear() {
    if (m_items.isEmpty())
        return;
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit countChanged();
}

void NotificationModel::trim(int limit) {
    if (m_items.size() <= limit)
        return;
    beginRemoveRows({}, limit, static_cast<int>(m_items.size()) - 1);
    m_items.resize(limit);
    endRemoveRows();
    emit countChanged();
}

// ---- Notifications ---------------------------------------------------------

namespace {

QString sanitizeBody(QString body) {
    // Spec markup is b, i, u, a, img. Images could load remote content.
    static const QRegularExpression img(QStringLiteral("<img[^>]*>"),
                                        QRegularExpression::CaseInsensitiveOption);
    body.remove(img);
    body.replace(u'\n', QStringLiteral("<br/>"));
    return body.trimmed();
}

QString processName(uint pid) {
    QFile f(QStringLiteral("/proc/%1/comm").arg(pid));
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()).trimmed() : QString();
}

} // namespace

Notifications* Notifications::instance() {
    static auto* n = new Notifications(QCoreApplication::instance());
    return n;
}

Notifications* Notifications::create(QQmlEngine*, QJSEngine*) {
    auto* n = instance();
    QJSEngine::setObjectOwnership(n, QJSEngine::CppOwnership);
    return n;
}

Notifications::Notifications(QObject* parent) : QObject(parent) {
    qDBusRegisterMetaType<QList<QVariantMap>>();
    new NotificationsAdaptor(this);
    new HistoryAdaptor(this);

    m_popupTimer.setSingleShot(true);
    connect(&m_popupTimer, &QTimer::timeout, this, &Notifications::endPopup);
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(1000);
    connect(&m_saveTimer, &QTimer::timeout, this, &Notifications::save);

    m_statePath = QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation) +
                  QLatin1String("/island-shell/notifications.json");
    m_cacheDir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) +
                 QLatin1String("/island-shell/notifications");
    load();
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this,
            &Notifications::save);
}

void Notifications::registerOnBus() {
    auto bus = QDBusConnection::sessionBus();
    // Always reachable for the history interface.
    bus.registerObject(QStringLiteral("/org/islandshell/Notifications"), this);

    const auto name = QStringLiteral("org.freedesktop.Notifications");
    if (!bus.isConnected()) {
        m_unavailableReason = QStringLiteral("no D-Bus session bus");
    } else if (bus.interface()->isServiceRegistered(name)) {
        const uint pid = bus.interface()->servicePid(name);
        m_unavailableReason =
            QStringLiteral("%1 is owned by %2 (pid %3); stop it to use island-shell's notifications")
                .arg(name, processName(pid), QString::number(pid));
    } else if (!bus.registerObject(QStringLiteral("/org/freedesktop/Notifications"), this) ||
               !bus.registerService(name)) {
        m_unavailableReason = QStringLiteral("could not register %1").arg(name);
    } else {
        m_available = true;
    }
    if (!m_available)
        qCWarning(lcNotif).noquote() << m_unavailableReason;
    emit availableChanged();
}

QVariantMap Notifications::current() const {
    const auto* n = m_history.find(m_current);
    return n ? NotificationModel::toMap(*n) : QVariantMap();
}

void Notifications::setPaused(bool paused) {
    if (m_paused == paused)
        return;
    m_paused = paused;
    if (m_current) {
        const auto* n = m_history.find(m_current);
        const int timeout = n ? popupTimeout(*n) : 0;
        if (m_paused)
            m_popupTimer.stop();
        else if (timeout > 0)
            m_popupTimer.start(std::max(timeout / 2, 2000));
    }
    emit pausedChanged();
}

int Notifications::popupTimeout(const NotificationData& n) const {
    if (n.urgency >= 2 && n.expireTimeout <= 0)
        return 0; // critical: until dismissed
    if (n.expireTimeout > 0)
        return std::clamp(n.expireTimeout, 1000, 60000);
    return Config::instance()->data().notifications.timeout;
}

void Notifications::showNext() {
    if (m_current || m_queue.isEmpty())
        return;
    m_current = m_queue.takeFirst();
    const auto* n = m_history.find(m_current);
    if (!n) {
        m_current = 0;
        showNext();
        return;
    }
    const int timeout = popupTimeout(*n);
    if (timeout > 0 && !m_paused)
        m_popupTimer.start(timeout);
    emit currentChanged();
    emit popupShown(timeout);
}

void Notifications::endPopup() {
    const uint id = m_current;
    if (!id)
        return;
    m_popupTimer.stop();
    m_current = 0;
    const auto* n = m_history.find(id);
    // Apps that set their own timeout (volume scripts, progress) want the
    // notification gone, not kept in history.
    if (n && (n->transient || n->expireTimeout > 0))
        close(id, Expired);
    emit currentChanged();
    showNext();
}

void Notifications::dismissPopup() {
    endPopup();
}

QStringList Notifications::GetCapabilities() {
    return {QStringLiteral("body"),        QStringLiteral("body-markup"),
            QStringLiteral("body-hyperlinks"), QStringLiteral("actions"),
            QStringLiteral("icon-static"), QStringLiteral("persistence"),
            QStringLiteral("inline-reply")};
}

QString Notifications::GetServerInformation(QString& vendor, QString& version,
                                            QString& spec_version) {
    vendor = QStringLiteral("island-shell");
    version = QCoreApplication::applicationVersion();
    spec_version = QStringLiteral("1.2");
    return QStringLiteral("island-shell");
}

uint Notifications::Notify(const QString& app_name, uint replaces_id, const QString& app_icon,
                           const QString& summary, const QString& body,
                           const QStringList& actions, const QVariantMap& hints,
                           int expire_timeout) {
    const bool replacing = replaces_id != 0 && m_history.find(replaces_id);
    const uint id = replacing ? replaces_id : m_nextId++;

    NotificationData n;
    n.id = id;
    n.appName = app_name;
    n.appIcon = app_icon;
    n.desktopEntry = hints.value(QStringLiteral("desktop-entry")).toString();
    n.summary = summary;
    n.body = sanitizeBody(body);
    n.actions = actions;
    n.expireTimeout = expire_timeout;
    n.urgency = hints.contains(QStringLiteral("urgency"))
                    ? std::clamp(hints.value(QStringLiteral("urgency")).toInt(), 0, 2)
                    : 1;
    n.transient = hints.value(QStringLiteral("transient")).toBool();
    n.resident = hints.value(QStringLiteral("resident")).toBool();
    n.inlineReply = actions.contains(QStringLiteral("inline-reply"));
    n.replyPlaceholder = hints.value(QStringLiteral("x-kde-reply-placeholder-text")).toString();
    n.time = QDateTime::currentDateTime();

    const auto& cfg = Config::instance()->data().notifications;
    const auto rule = applyNotificationRules(cfg.rules, app_name.toStdString(),
                                             n.desktopEntry.toStdString(), n.urgency,
                                             ShellControl::instance()->dnd());
    if (rule.drop) {
        qCDebug(lcNotif) << "muted by rule:" << app_name;
        return id;
    }
    n.urgency = rule.urgency;

    for (const auto* key : {"image-data", "image_data", "icon_data"}) {
        const auto v = hints.value(QLatin1String(key));
        if (v.isValid()) {
            n.image = storeImage(id, v);
            n.imageCached = !n.image.isEmpty();
            break;
        }
    }
    if (n.image.isEmpty()) {
        n.image = hints.value(QStringLiteral("image-path")).toString();
        if (n.image.isEmpty())
            n.image = hints.value(QStringLiteral("image_path")).toString();
        if (n.image.startsWith(QLatin1String("file://")))
            n.image = QUrl(n.image).toLocalFile();
    }

    if (const auto* old = m_history.find(id); old && old->imageCached && old->image != n.image)
        QFile::remove(old->image);
    m_history.upsert(n);

    if (rule.popup) {
        if (id == m_current) {
            const int timeout = popupTimeout(n);
            if (timeout > 0 && !m_paused)
                m_popupTimer.start(timeout);
            emit currentChanged();
        } else if (!m_queue.contains(id)) {
            m_queue.append(id);
        }
        showNext();
    } else if (n.transient) {
        removeEverywhere(id);
    }

    // Trim history, keeping anything still waiting to pop up.
    const int limit = cfg.historyLimit;
    while (m_history.rowCount() > limit) {
        const auto& last = m_history.items().last();
        if (last.id == m_current || m_queue.contains(last.id))
            break;
        removeEverywhere(last.id);
    }
    scheduleSave();
    return id;
}

QString Notifications::storeImage(uint id, const QVariant& imageData) {
    // (iiibiiay): width, height, rowstride, has_alpha, bits, channels, data
    const auto arg = imageData.value<QDBusArgument>();
    int width = 0, height = 0, rowstride = 0, bits = 0, channels = 0;
    bool alpha = false;
    QByteArray data;
    arg.beginStructure();
    arg >> width >> height >> rowstride >> alpha >> bits >> channels >> data;
    arg.endStructure();
    if (width <= 0 || height <= 0 || bits != 8 || (channels != 3 && channels != 4) ||
        data.size() < qsizetype(rowstride) * (height - 1) + qsizetype(width) * channels)
        return {};
    const QImage image(reinterpret_cast<const uchar*>(data.constData()), width, height, rowstride,
                       alpha ? QImage::Format_RGBA8888 : QImage::Format_RGB888);
    QDir().mkpath(m_cacheDir);
    const auto path = QStringLiteral("%1/%2-%3.png")
                          .arg(m_cacheDir)
                          .arg(id)
                          .arg(QDateTime::currentMSecsSinceEpoch());
    return image.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation).save(path)
               ? path
               : QString();
}

void Notifications::CloseNotification(uint id) {
    close(id, ClosedByCall);
}

void Notifications::close(uint id, CloseReason reason) {
    if (!m_history.find(id))
        return;
    removeEverywhere(id);
    emit NotificationClosed(id, reason);
}

void Notifications::removeEverywhere(uint id) {
    if (const auto* n = m_history.find(id); n && n->imageCached)
        QFile::remove(n->image);
    m_queue.removeAll(id);
    m_history.remove(id);
    if (m_current == id) {
        m_current = 0;
        m_popupTimer.stop();
        emit currentChanged();
        showNext();
    }
    scheduleSave();
}

void Notifications::invokeAction(uint id, const QString& key) {
    const auto* n = m_history.find(id);
    if (!n)
        return;
    const bool resident = n->resident;
    emit ActionInvoked(id, key);
    if (resident)
        endPopup();
    else
        close(id, Dismissed);
}

void Notifications::reply(uint id, const QString& text) {
    const auto* n = m_history.find(id);
    if (!n || text.trimmed().isEmpty())
        return;
    emit NotificationReplied(id, text);
    if (n->resident)
        endPopup();
    else
        close(id, Dismissed);
}

void Notifications::dismiss(uint id) {
    close(id, Dismissed);
}

void Notifications::clearHistory() {
    const auto items = m_history.items();
    for (const auto& n : items)
        close(n.id, Dismissed);
}

QList<QVariantMap> Notifications::List() {
    QList<QVariantMap> out;
    for (const auto& n : m_history.items()) {
        out.append({
            {QStringLiteral("id"), n.id},
            {QStringLiteral("app"), n.appName},
            {QStringLiteral("summary"), n.summary},
            {QStringLiteral("body"), n.body},
            {QStringLiteral("time"), n.time.toString(Qt::ISODate)},
            {QStringLiteral("urgency"), n.urgency},
        });
    }
    return out;
}

void Notifications::Clear() {
    clearHistory();
}

void Notifications::Dismiss(uint id) {
    dismiss(id);
}

void Notifications::scheduleSave() {
    m_saveTimer.start();
}

void Notifications::load() {
    QFile f(m_statePath);
    if (!f.open(QIODevice::ReadOnly))
        return;
    const auto array = QJsonDocument::fromJson(f.readAll()).array();
    // Saved newest first; upsert prepends, so walk backwards.
    for (auto i = array.size() - 1; i >= 0; --i) {
        const auto o = array[i].toObject();
        NotificationData n;
        n.id = static_cast<uint>(o[u"id"].toInteger());
        if (n.id == 0)
            continue;
        n.appName = o[u"appName"].toString();
        n.appIcon = o[u"appIcon"].toString();
        n.desktopEntry = o[u"desktopEntry"].toString();
        n.summary = o[u"summary"].toString();
        n.body = o[u"body"].toString();
        n.image = o[u"image"].toString();
        n.imageCached = o[u"imageCached"].toBool();
        n.urgency = o[u"urgency"].toInt(1);
        n.time = QDateTime::fromString(o[u"time"].toString(), Qt::ISODate);
        // Actions and replies need the sender, which is gone after a restart.
        m_history.upsert(n);
        m_nextId = std::max(m_nextId, n.id + 1);
    }
    qCInfo(lcNotif) << "loaded" << m_history.rowCount() << "notifications from history";
}

void Notifications::save() {
    QJsonArray array;
    for (const auto& n : m_history.items()) {
        if (n.transient)
            continue;
        array.append(QJsonObject{
            {u"id"_s, static_cast<qint64>(n.id)},
            {u"appName"_s, n.appName},
            {u"appIcon"_s, n.appIcon},
            {u"desktopEntry"_s, n.desktopEntry},
            {u"summary"_s, n.summary},
            {u"body"_s, n.body},
            {u"image"_s, n.image},
            {u"imageCached"_s, n.imageCached},
            {u"urgency"_s, n.urgency},
            {u"time"_s, n.time.toString(Qt::ISODate)},
        });
    }
    QDir().mkpath(QFileInfo(m_statePath).path());
    QSaveFile f(m_statePath);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(array).toJson(QJsonDocument::Compact));
        f.commit();
    }
}

} // namespace island
