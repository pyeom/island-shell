#include "bluetooth.h"

#include "dbusproperties.h"
#include "dbusutil.h"

#include <QCoreApplication>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QJSEngine>
#include <QLoggingCategory>

#include <algorithm>

Q_LOGGING_CATEGORY(lcBt, "island.bluetooth")

namespace island {

namespace {
const QString kService = QStringLiteral("org.bluez");
const QString kAdapter = QStringLiteral("org.bluez.Adapter1");
const QString kDevice = QStringLiteral("org.bluez.Device1");
const QString kBattery = QStringLiteral("org.bluez.Battery1");
const QString kObjectManager = QStringLiteral("org.freedesktop.DBus.ObjectManager");

// a{oa{sa{sv}}} → path -> interface names, read by hand so no custom
// metatypes are needed.
QHash<QString, QStringList> managedObjects(const QDBusArgument& arg) {
    QHash<QString, QStringList> out;
    arg.beginMap();
    while (!arg.atEnd()) {
        QDBusObjectPath path;
        arg.beginMapEntry();
        arg >> path;
        QStringList interfaces;
        arg.beginMap();
        while (!arg.atEnd()) {
            QString iface;
            QVariantMap props;
            arg.beginMapEntry();
            arg >> iface >> props;
            arg.endMapEntry();
            interfaces.append(iface);
        }
        arg.endMap();
        arg.endMapEntry();
        out.insert(path.path(), interfaces);
    }
    arg.endMap();
    return out;
}

QStringList interfaceNames(const QDBusArgument& arg) {
    QStringList out;
    arg.beginMap();
    while (!arg.atEnd()) {
        QString iface;
        QVariantMap props;
        arg.beginMapEntry();
        arg >> iface >> props;
        arg.endMapEntry();
        out.append(iface);
    }
    arg.endMap();
    return out;
}

} // namespace

Bluetooth* Bluetooth::create(QQmlEngine*, QJSEngine*) {
    static auto* bt = new Bluetooth(QCoreApplication::instance());
    QJSEngine::setObjectOwnership(bt, QJSEngine::CppOwnership);
    return bt;
}

Bluetooth::Bluetooth(QObject* parent) : QObject(parent) {
    auto bus = QDBusConnection::systemBus();
    m_watcher.setConnection(bus);
    m_watcher.addWatchedService(kService);
    connect(&m_watcher, &QDBusServiceWatcher::serviceRegistered, this, &Bluetooth::load);
    connect(&m_watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this] {
        clear();
        emit changed();
    });
    // Taking the raw message avoids registering a{sa{sv}} metatypes for the
    // slot signature (which QtDBus failed to match before).
    bus.connect(kService, QStringLiteral("/"), kObjectManager, QStringLiteral("InterfacesAdded"), this,
                SLOT(onInterfacesAdded(QDBusMessage)));
    bus.connect(kService, QStringLiteral("/"), kObjectManager, QStringLiteral("InterfacesRemoved"), this,
                SLOT(onInterfacesRemoved(QDBusMessage)));
    if (bus.interface()->isServiceRegistered(kService))
        load();
}

void Bluetooth::clear() {
    delete m_adapter;
    for (const auto& d : std::as_const(m_devices)) {
        delete d.device;
        delete d.battery;
    }
    m_devices.clear();
}

void Bluetooth::load() {
    clear();
    auto msg = QDBusMessage::createMethodCall(kService, QStringLiteral("/"), kObjectManager,
                                              QStringLiteral("GetManagedObjects"));
    auto* watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
        w->deleteLater();
        const auto reply = w->reply();
        if (reply.type() != QDBusMessage::ReplyMessage)
            return;
        const auto objects = managedObjects(reply.arguments().value(0).value<QDBusArgument>());
        for (auto it = objects.cbegin(); it != objects.cend(); ++it)
            addObject(it.key(), it.value());
        emit changed();
    });
}

void Bluetooth::addObject(const QString& path, const QStringList& interfaces) {
    auto bus = QDBusConnection::systemBus();
    if (interfaces.contains(kAdapter) && !m_adapter) {
        m_adapter = new DBusProperties(bus, kService, path, kAdapter, this);
        connect(m_adapter, &DBusProperties::changed, this, &Bluetooth::changed);
        // A list opened before the adapter loaded asked for discovery.
        connect(m_adapter, &DBusProperties::changed, this, [this] {
            if (m_wantDiscovery && m_adapter->ready() && !discovering() && powered())
                call(m_adapter->path(), kAdapter, QStringLiteral("StartDiscovery"));
        }, Qt::SingleShotConnection);
    }
    if (interfaces.contains(kDevice) && !m_devices.contains(path)) {
        auto* dev = new DBusProperties(bus, kService, path, kDevice, this);
        connect(dev, &DBusProperties::changed, this, &Bluetooth::changed);
        m_devices.insert(path, {dev, nullptr});
    }
    if (interfaces.contains(kBattery) && m_devices.contains(path) && !m_devices[path].battery) {
        auto* battery = new DBusProperties(bus, kService, path, kBattery, this);
        connect(battery, &DBusProperties::changed, this, &Bluetooth::changed);
        m_devices[path].battery = battery;
    }
}

void Bluetooth::onInterfacesAdded(const QDBusMessage& message) {
    const auto args = message.arguments();
    const auto path = dbus::toPath(args.value(0));
    addObject(path, interfaceNames(args.value(1).value<QDBusArgument>()));
    emit changed();
}

void Bluetooth::onInterfacesRemoved(const QDBusMessage& message) {
    const auto args = message.arguments();
    const auto path = dbus::toPath(args.value(0));
    const auto interfaces = args.value(1).toStringList();
    if (interfaces.contains(kDevice)) {
        const auto d = m_devices.take(path);
        delete d.device;
        delete d.battery;
    } else if (interfaces.contains(kBattery) && m_devices.contains(path)) {
        delete m_devices[path].battery;
        m_devices[path].battery = nullptr;
    }
    if (interfaces.contains(kAdapter) && m_adapter && m_adapter->path() == path)
        delete m_adapter;
    emit changed();
}

bool Bluetooth::available() const {
    return m_adapter && m_adapter->ready();
}

bool Bluetooth::powered() const {
    return m_adapter && m_adapter->value(QStringLiteral("Powered")).toBool();
}

bool Bluetooth::discovering() const {
    return m_adapter && m_adapter->value(QStringLiteral("Discovering")).toBool();
}

QStringList Bluetooth::connectedDevices() const {
    QStringList names;
    for (const auto& d : m_devices)
        if (d.device->value(QStringLiteral("Connected")).toBool())
            names.append(d.device->value(QStringLiteral("Alias")).toString());
    names.sort(Qt::CaseInsensitive);
    return names;
}

QVariantList Bluetooth::devices() const {
    QVariantList out;
    for (auto it = m_devices.cbegin(); it != m_devices.cend(); ++it) {
        const auto* d = it->device;
        const auto name = d->value(QStringLiteral("Alias")).toString();
        if (name.isEmpty())
            continue;
        out.append(QVariantMap{
            {QStringLiteral("path"), it.key()},
            {QStringLiteral("name"), name},
            {QStringLiteral("icon"), d->value(QStringLiteral("Icon")).toString()},
            {QStringLiteral("paired"), d->value(QStringLiteral("Paired")).toBool()},
            {QStringLiteral("connected"), d->value(QStringLiteral("Connected")).toBool()},
            {QStringLiteral("battery"),
             it->battery ? static_cast<int>(it->battery->value(QStringLiteral("Percentage")).toUInt()) : -1},
        });
    }
    std::ranges::sort(out, [](const QVariant& a, const QVariant& b) {
        const auto ma = a.toMap();
        const auto mb = b.toMap();
        for (const auto* key : {"connected", "paired"}) {
            const bool x = ma.value(QLatin1String(key)).toBool();
            const bool y = mb.value(QLatin1String(key)).toBool();
            if (x != y)
                return x;
        }
        return QString::localeAwareCompare(ma.value(QStringLiteral("name")).toString(),
                                           mb.value(QStringLiteral("name")).toString()) < 0;
    });
    return out;
}

void Bluetooth::setPowered(bool powered) {
    if (m_adapter)
        m_adapter->set(QStringLiteral("Powered"), powered);
}

void Bluetooth::call(const QString& path, const QString& iface, const QString& method) {
    auto msg = QDBusMessage::createMethodCall(kService, path, iface, method);
    auto* watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [method](QDBusPendingCallWatcher* w) {
        w->deleteLater();
        if (w->isError())
            qCWarning(lcBt).noquote() << method << "failed:" << w->error().message();
    });
}

void Bluetooth::setDiscovering(bool on) {
    m_wantDiscovery = on;
    if (!m_adapter || !m_adapter->ready() || on == discovering())
        return;
    call(m_adapter->path(), kAdapter, on ? QStringLiteral("StartDiscovery") : QStringLiteral("StopDiscovery"));
}

void Bluetooth::connectDevice(const QString& path) {
    const auto it = m_devices.constFind(path);
    if (it == m_devices.cend())
        return;
    if (!it->device->value(QStringLiteral("Paired")).toBool()) {
        // Pair, then connect once Paired turns true.
        auto msg = QDBusMessage::createMethodCall(kService, path, kDevice, QStringLiteral("Pair"));
        auto* watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, path](QDBusPendingCallWatcher* w) {
            w->deleteLater();
            if (w->isError()) {
                qCWarning(lcBt).noquote() << "Pair failed:" << w->error().message();
                return;
            }
            call(path, kDevice, QStringLiteral("Connect"));
        });
        return;
    }
    call(path, kDevice, QStringLiteral("Connect"));
}

void Bluetooth::disconnectDevice(const QString& path) {
    if (m_devices.contains(path))
        call(path, kDevice, QStringLiteral("Disconnect"));
}

} // namespace island
