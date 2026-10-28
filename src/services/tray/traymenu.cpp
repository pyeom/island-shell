#include "traymenu.h"

#include "dbusutil.h"
#include "icons.h"
#include "tray.h"
#include "trayparse.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusVariant>
#include <QDateTime>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcTrayMenu, "island.tray.menu")

using namespace Qt::StringLiterals;

namespace island {

namespace {

const auto kInterface = u"com.canonical.dbusmenu"_s;

// One (ia{sv}av) layout node and its children.
QVariantMap readNode(const QDBusArgument& arg, QStringList& images) {
    int id = 0;
    QVariantMap props;
    QVariantList children;
    arg.beginStructure();
    arg >> id >> props;
    arg.beginArray();
    while (!arg.atEnd()) {
        QDBusVariant child;
        arg >> child;
        const auto value = child.variant();
        if (!value.canConvert<QDBusArgument>())
            continue;
        auto entry = readNode(value.value<QDBusArgument>(), images);
        if (!entry.isEmpty())
            children.append(entry);
    }
    arg.endArray();
    arg.endStructure();

    const auto prop = [&](const char* name, const QVariant& fallback = {}) {
        const auto v = props.value(QLatin1String(name));
        return v.isValid() ? dbus::unwrap(v) : fallback;
    };
    if (!prop("visible", true).toBool())
        return {};

    const auto iconName = prop("icon-name").toString();
    QString icon = !iconName.isEmpty() && Icons::exists(iconName) ? Icons::source(iconName) : QString();
    if (icon.isEmpty()) {
        const auto png = prop("icon-data").toByteArray();
        const auto image = png.isEmpty() ? QImage() : QImage::fromData(png);
        if (!image.isNull()) {
            icon = TrayImageProvider::store(image);
            images.append(icon);
        }
    }
    const auto toggle = prop("toggle-type").toString();
    return {
        {u"id"_s, id},
        {u"label"_s, trayMenuLabel(prop("label").toString())},
        {u"enabled"_s, prop("enabled", true).toBool()},
        {u"separator"_s, prop("type").toString() == u"separator"},
        {u"toggle"_s, toggle == u"checkmark" || toggle == u"radio" ? toggle : QString()},
        {u"checked"_s, prop("toggle-state", 0).toInt() == 1},
        {u"icon"_s, icon},
        {u"submenu"_s, prop("children-display").toString() == u"submenu" || !children.isEmpty()},
        {u"children"_s, children},
    };
}

void dropImages(QStringList& images) {
    for (const auto& source : std::as_const(images))
        TrayImageProvider::drop(source);
    images.clear();
}

} // namespace

TrayMenu::TrayMenu(QObject* parent) : QObject(parent) {
    m_fetch.setSingleShot(true);
    m_fetch.setInterval(30); // LayoutUpdated and property updates come in bursts
    connect(&m_fetch, &QTimer::timeout, this, &TrayMenu::fetch);
}

TrayMenu::~TrayMenu() {
    disconnectSource();
    dropImages(m_images);
}

void TrayMenu::setService(const QString& service) {
    if (service == m_service)
        return;
    disconnectSource();
    m_service = service;
    connectSource();
    emit sourceChanged();
}

void TrayMenu::setPath(const QString& path) {
    if (path == m_path)
        return;
    disconnectSource();
    m_path = path;
    connectSource();
    emit sourceChanged();
}

void TrayMenu::connectSource() {
    m_entries.clear();
    emit entriesChanged();
    if (m_service.isEmpty() || m_path.isEmpty())
        return;
    auto bus = QDBusConnection::sessionBus();
    bus.connect(m_service, m_path, kInterface, u"LayoutUpdated"_s, this, SLOT(onLayoutUpdated()));
    bus.connect(m_service, m_path, kInterface, u"ItemsPropertiesUpdated"_s, this, SLOT(onLayoutUpdated()));
    m_connected = true;
    setLoading(true);
    aboutToShow(0);
}

void TrayMenu::disconnectSource() {
    if (!m_connected)
        return;
    auto bus = QDBusConnection::sessionBus();
    bus.disconnect(m_service, m_path, kInterface, u"LayoutUpdated"_s, this, SLOT(onLayoutUpdated()));
    bus.disconnect(m_service, m_path, kInterface, u"ItemsPropertiesUpdated"_s, this, SLOT(onLayoutUpdated()));
    m_connected = false;
}

void TrayMenu::setLoading(bool loading) {
    if (loading == m_loading)
        return;
    m_loading = loading;
    emit loadingChanged();
}

void TrayMenu::onLayoutUpdated() {
    m_fetch.start();
}

void TrayMenu::aboutToShow(int id) {
    if (!m_connected)
        return;
    auto msg = QDBusMessage::createMethodCall(m_service, m_path, kInterface, u"AboutToShow"_s);
    msg.setArguments({id});
    auto* watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(msg), this);
    // Fetched either way: not every app implements AboutToShow, and the
    // first layout is needed regardless.
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, id](QDBusPendingCallWatcher* w) {
        w->deleteLater();
        if (id == 0 || (!w->isError() && w->reply().arguments().value(0).toBool()))
            m_fetch.start();
    });
}

void TrayMenu::fetch() {
    if (!m_connected)
        return;
    auto msg = QDBusMessage::createMethodCall(m_service, m_path, kInterface, u"GetLayout"_s);
    msg.setArguments({0, -1, QStringList()});
    auto* watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
        w->deleteLater();
        setLoading(false);
        const auto reply = w->reply();
        if (w->isError() || reply.arguments().size() < 2) {
            qCWarning(lcTrayMenu).noquote() << m_service << m_path << "GetLayout ->" << w->error().message();
            return;
        }
        const auto layout = reply.arguments().at(1);
        if (!layout.canConvert<QDBusArgument>())
            return;
        QStringList images;
        const auto root = readNode(layout.value<QDBusArgument>(), images);
        dropImages(m_images);
        m_images = images;
        m_entries = root.value(u"children"_s).toList();
        emit entriesChanged();
    });
}

void TrayMenu::activate(int id) {
    if (!m_connected)
        return;
    auto msg = QDBusMessage::createMethodCall(m_service, m_path, kInterface, u"Event"_s);
    msg.setArguments({id, u"clicked"_s, QVariant::fromValue(QDBusVariant(0)),
                      static_cast<uint>(QDateTime::currentSecsSinceEpoch())});
    QDBusConnection::sessionBus().asyncCall(msg);
}

} // namespace island
