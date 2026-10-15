#include "bluetooth.h"

#include "dbusproperties.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QJSEngine>

namespace island {

namespace {
const QString kService = QStringLiteral("org.bluez");
const QString kAdapter = QStringLiteral("org.bluez.Adapter1");
const QString kDevice = QStringLiteral("org.bluez.Device1");
const QString kObjectManager = QStringLiteral("org.freedesktop.DBus.ObjectManager");
} // namespace

Bluetooth* Bluetooth::create(QQmlEngine*, QJSEngine*) {
    static auto* bt = new Bluetooth(QCoreApplication::instance());
    QJSEngine::setObjectOwnership(bt, QJSEngine::CppOwnership);
    return bt;
}

Bluetooth::Bluetooth(QObject* parent) : QObject(parent) {
    qDBusRegisterMetaType<InterfaceMap>();
    qDBusRegisterMetaType<ManagedObjects>();

    auto bus = QDBusConnection::systemBus();
    m_watcher.setConnection(bus);
    m_watcher.addWatchedService(kService);
    connect(&m_watcher, &QDBusServiceWatcher::serviceRegistered, this, &Bluetooth::load);
    connect(&m_watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this] {
        clear();
        emit changed();
    });
    bus.connect(kService, QStringLiteral("/"), kObjectManager, QStringLiteral("InterfacesAdded"),
                this, SLOT(onInterfacesAdded(QDBusObjectPath, island::InterfaceMap)));
    bus.connect(kService, QStringLiteral("/"), kObjectManager, QStringLiteral("InterfacesRemoved"),
                this, SLOT(onInterfacesRemoved(QDBusObjectPath, QStringList)));
    if (bus.interface()->isServiceRegistered(kService))
        load();
}

void Bluetooth::clear() {
    delete m_adapter;
    qDeleteAll(m_devices);
    m_devices.clear();
}

void Bluetooth::load() {
    clear();
    auto msg = QDBusMessage::createMethodCall(kService, QStringLiteral("/"), kObjectManager,
                                              QStringLiteral("GetManagedObjects"));
    auto* watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
        QDBusPendingReply<ManagedObjects> reply = *w;
        w->deleteLater();
        if (reply.isError())
            return;
        const auto objects = reply.value();
        for (auto it = objects.cbegin(); it != objects.cend(); ++it)
            addObject(it.key().path(), it.value());
        emit changed();
    });
}

void Bluetooth::addObject(const QString& path, const InterfaceMap& interfaces) {
    auto bus = QDBusConnection::systemBus();
    if (interfaces.contains(kAdapter) && !m_adapter) {
        m_adapter = new DBusProperties(bus, kService, path, kAdapter, this);
        connect(m_adapter, &DBusProperties::changed, this, &Bluetooth::changed);
    }
    if (interfaces.contains(kDevice) && !m_devices.contains(path)) {
        auto* dev = new DBusProperties(bus, kService, path, kDevice, this);
        connect(dev, &DBusProperties::changed, this, &Bluetooth::changed);
        m_devices.insert(path, dev);
    }
}

void Bluetooth::onInterfacesAdded(const QDBusObjectPath& path, const InterfaceMap& interfaces) {
    addObject(path.path(), interfaces);
    emit changed();
}

void Bluetooth::onInterfacesRemoved(const QDBusObjectPath& path, const QStringList& interfaces) {
    if (interfaces.contains(kDevice))
        delete m_devices.take(path.path());
    if (interfaces.contains(kAdapter) && m_adapter && m_adapter->path() == path.path())
        delete m_adapter;
    emit changed();
}

bool Bluetooth::available() const {
    return m_adapter && m_adapter->ready();
}

bool Bluetooth::powered() const {
    return m_adapter && m_adapter->value(QStringLiteral("Powered")).toBool();
}

QStringList Bluetooth::connectedDevices() const {
    QStringList names;
    for (const auto* d : m_devices)
        if (d->value(QStringLiteral("Connected")).toBool())
            names.append(d->value(QStringLiteral("Alias")).toString());
    names.sort(Qt::CaseInsensitive);
    return names;
}

void Bluetooth::setPowered(bool powered) {
    if (m_adapter)
        m_adapter->set(QStringLiteral("Powered"), powered);
}

} // namespace island
