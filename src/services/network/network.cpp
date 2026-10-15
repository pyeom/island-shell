#include "network.h"

#include "dbusproperties.h"
#include "dbusutil.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QJSEngine>

namespace island {

namespace {
const QString kService = QStringLiteral("org.freedesktop.NetworkManager");
const QString kRootPath = QStringLiteral("/org/freedesktop/NetworkManager");
constexpr uint kStateConnectedSite = 60;
constexpr uint kDeviceTypeWifi = 2;
} // namespace

Network* Network::create(QQmlEngine*, QJSEngine*) {
    static auto* network = new Network(QCoreApplication::instance());
    QJSEngine::setObjectOwnership(network, QJSEngine::CppOwnership);
    return network;
}

Network::Network(QObject* parent) : QObject(parent) {
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(50);
    connect(&m_debounce, &QTimer::timeout, this, &Network::sync);
    m_root = new DBusProperties(QDBusConnection::systemBus(), kService, kRootPath, kService, this);
    connect(m_root, &DBusProperties::changed, &m_debounce, qOverload<>(&QTimer::start));
}

void Network::sync() {
    syncDevices();
    syncActive();
    emit changed();
}

void Network::syncDevices() {
    const auto paths = dbus::toPaths(m_root->value(QStringLiteral("AllDevices")));
    for (auto it = m_devices.begin(); it != m_devices.end();) {
        if (!paths.contains(it.key())) {
            it.value()->deleteLater();
            it = m_devices.erase(it);
        } else {
            ++it;
        }
    }
    for (const auto& path : paths) {
        if (m_devices.contains(path))
            continue;
        auto* dev = new DBusProperties(QDBusConnection::systemBus(), kService, path,
                                       kService + QLatin1String(".Device"), this);
        connect(dev, &DBusProperties::changed, &m_debounce, qOverload<>(&QTimer::start));
        m_devices.insert(path, dev);
    }
    m_hasWifi = std::ranges::any_of(m_devices, [](const DBusProperties* d) {
        return d->value(QStringLiteral("DeviceType")).toUInt() == kDeviceTypeWifi;
    });
}

void Network::syncActive() {
    const auto path = dbus::toPath(m_root->value(QStringLiteral("PrimaryConnection")));
    const bool none = path.isEmpty() || path == u"/";
    if (none) {
        delete m_active;
    } else if (!m_active || m_active->path() != path) {
        delete m_active;
        m_active = new DBusProperties(QDBusConnection::systemBus(), kService, path,
                                      kService + QLatin1String(".Connection.Active"), this);
        connect(m_active, &DBusProperties::changed, &m_debounce, qOverload<>(&QTimer::start));
    }
    syncWireless();
}

void Network::syncWireless() {
    QString devicePath;
    if (m_active && primaryType() == u"wifi") {
        const auto devices = dbus::toPaths(m_active->value(QStringLiteral("Devices")));
        devicePath = devices.value(0);
    }
    if (devicePath.isEmpty()) {
        delete m_wireless;
        delete m_ap;
        m_ssid.clear();
        m_strength = 0;
        return;
    }
    if (!m_wireless || m_wireless->path() != devicePath) {
        delete m_wireless;
        m_wireless = new DBusProperties(QDBusConnection::systemBus(), kService, devicePath,
                                        kService + QLatin1String(".Device.Wireless"), this);
        connect(m_wireless, &DBusProperties::changed, &m_debounce, qOverload<>(&QTimer::start));
    }
    const auto apPath = dbus::toPath(m_wireless->value(QStringLiteral("ActiveAccessPoint")));
    if (apPath.isEmpty() || apPath == u"/") {
        delete m_ap;
    } else if (!m_ap || m_ap->path() != apPath) {
        delete m_ap;
        m_ap = new DBusProperties(QDBusConnection::systemBus(), kService, apPath,
                                  kService + QLatin1String(".AccessPoint"), this);
        connect(m_ap, &DBusProperties::changed, &m_debounce, qOverload<>(&QTimer::start));
    }
    m_ssid = m_ap ? QString::fromUtf8(m_ap->value(QStringLiteral("Ssid")).toByteArray()) : QString();
    m_strength = m_ap ? static_cast<int>(m_ap->value(QStringLiteral("Strength")).toUInt()) : 0;
}

bool Network::available() const {
    return m_root->ready();
}

bool Network::connected() const {
    return m_root->value(QStringLiteral("State")).toUInt() >= kStateConnectedSite;
}

QString Network::primaryType() const {
    const auto type = m_root->value(QStringLiteral("PrimaryConnectionType")).toString();
    if (type.isEmpty())
        return {};
    if (type == u"802-11-wireless")
        return QStringLiteral("wifi");
    if (type == u"802-3-ethernet")
        return QStringLiteral("ethernet");
    if (type == u"vpn" || type == u"wireguard")
        return QStringLiteral("vpn");
    return QStringLiteral("other");
}

QString Network::connectionName() const {
    return m_active ? m_active->value(QStringLiteral("Id")).toString() : QString();
}

bool Network::wifiEnabled() const {
    return m_root->value(QStringLiteral("WirelessEnabled")).toBool();
}

QString Network::iconName() const {
    if (!connected())
        return QStringLiteral("network-offline-symbolic");
    const auto type = primaryType();
    if (type == u"wifi") {
        const char* level = m_strength > 75 ? "excellent" : m_strength > 50 ? "good"
                            : m_strength > 25 ? "ok" : "weak";
        return QStringLiteral("network-wireless-signal-%1-symbolic").arg(QLatin1String(level));
    }
    if (type == u"vpn")
        return QStringLiteral("network-vpn-symbolic");
    return QStringLiteral("network-wired-symbolic");
}

void Network::setWifiEnabled(bool enabled) {
    m_root->set(QStringLiteral("WirelessEnabled"), enabled);
}

} // namespace island
