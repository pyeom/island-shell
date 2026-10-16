#include "network.h"

#include "dbusproperties.h"
#include "dbusutil.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusMetaType>
#include <QJSEngine>

using NmSettings = QMap<QString, QVariantMap>;
Q_DECLARE_METATYPE(NmSettings)

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
    qDBusRegisterMetaType<NmSettings>();
    m_networksDebounce.setSingleShot(true);
    m_networksDebounce.setInterval(300);
    connect(&m_networksDebounce, &QTimer::timeout, this, &Network::refreshNetworks);
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
    if (m_hasWifi && m_watching)
        m_networksDebounce.start();
}

void Network::setWatching(bool watching) {
    m_watching = watching;
    if (watching) {
        refreshNetworks();
        scan();
    }
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

QString Network::wifiDevicePath() const {
    for (auto it = m_devices.cbegin(); it != m_devices.cend(); ++it)
        if (it.value()->value(QStringLiteral("DeviceType")).toUInt() == kDeviceTypeWifi)
            return it.key();
    return {};
}

namespace {

QDBusMessage nmCall(const QString& path, const QString& iface, const QString& method,
                    const QVariantList& args = {}) {
    auto msg = QDBusMessage::createMethodCall(kService, path, iface, method);
    msg.setArguments(args);
    return msg;
}

// Properties of one object, synchronously but with a short timeout: used
// only while building the network list, a handful of calls.
QVariantMap getAll(const QString& path, const QString& iface) {
    auto msg = QDBusMessage::createMethodCall(kService, path, QStringLiteral("org.freedesktop.DBus.Properties"),
                                              QStringLiteral("GetAll"));
    msg.setArguments({iface});
    const auto reply = QDBusConnection::systemBus().call(msg, QDBus::Block, 1500);
    return reply.type() == QDBusMessage::ReplyMessage ? dbus::toMap(reply.arguments().value(0)) : QVariantMap();
}

} // namespace

void Network::refreshNetworks() {
    const auto device = wifiDevicePath();
    if (device.isEmpty())
        return;
    auto bus = QDBusConnection::systemBus();

    // Saved Wi-Fi profiles by SSID.
    m_knownBySsid.clear();
    const auto list = bus.call(nmCall(QStringLiteral("/org/freedesktop/NetworkManager/Settings"),
                                      kService + QLatin1String(".Settings"), QStringLiteral("ListConnections")),
                               QDBus::Block, 1500);
    for (const auto& path : dbus::cast<QList<QDBusObjectPath>>(list.arguments().value(0))) {
        const auto reply = bus.call(nmCall(path.path(), kService + QLatin1String(".Settings.Connection"),
                                           QStringLiteral("GetSettings")),
                                    QDBus::Block, 1500);
        const auto settings = dbus::cast<NmSettings>(reply.arguments().value(0));
        const auto wifi = settings.value(QStringLiteral("802-11-wireless"));
        const auto ssid = QString::fromUtf8(wifi.value(QStringLiteral("ssid")).toByteArray());
        if (!ssid.isEmpty())
            m_knownBySsid.insert(ssid, path.path());
    }

    // Visible access points, strongest per SSID.
    const auto aps = bus.call(nmCall(device, kService + QLatin1String(".Device.Wireless"),
                                     QStringLiteral("GetAllAccessPoints")),
                              QDBus::Block, 1500);
    struct Ap {
        QString path;
        int strength;
        bool secured;
    };
    QHash<QString, Ap> best;
    for (const auto& ap : dbus::cast<QList<QDBusObjectPath>>(aps.arguments().value(0))) {
        const auto props = getAll(ap.path(), kService + QLatin1String(".AccessPoint"));
        const auto ssid = QString::fromUtf8(props.value(QStringLiteral("Ssid")).toByteArray());
        if (ssid.isEmpty())
            continue; // hidden network
        const int strength = static_cast<int>(props.value(QStringLiteral("Strength")).toUInt());
        const bool secured = props.value(QStringLiteral("WpaFlags")).toUInt() != 0 ||
                             props.value(QStringLiteral("RsnFlags")).toUInt() != 0 ||
                             (props.value(QStringLiteral("Flags")).toUInt() & 0x1);
        if (!best.contains(ssid) || best[ssid].strength < strength)
            best.insert(ssid, {ap.path(), strength, secured});
    }

    m_apBySsid.clear();
    QVariantList out;
    for (auto it = best.cbegin(); it != best.cend(); ++it) {
        m_apBySsid.insert(it.key(), it->path);
        out.append(QVariantMap{
            {QStringLiteral("ssid"), it.key()},
            {QStringLiteral("strength"), it->strength},
            {QStringLiteral("secured"), it->secured},
            {QStringLiteral("known"), m_knownBySsid.contains(it.key())},
            {QStringLiteral("active"), it.key() == m_ssid && connected()},
        });
    }
    std::ranges::sort(out, [](const QVariant& a, const QVariant& b) {
        const auto ma = a.toMap();
        const auto mb = b.toMap();
        if (ma.value(QStringLiteral("active")) != mb.value(QStringLiteral("active")))
            return ma.value(QStringLiteral("active")).toBool();
        return ma.value(QStringLiteral("strength")).toInt() > mb.value(QStringLiteral("strength")).toInt();
    });
    m_networks = out;
    emit networksChanged();
}

void Network::scan() {
    const auto device = wifiDevicePath();
    if (device.isEmpty())
        return;
    QDBusConnection::systemBus().asyncCall(nmCall(device, kService + QLatin1String(".Device.Wireless"),
                                                  QStringLiteral("RequestScan"), {QVariantMap()}));
    // Results arrive as access points come and go; refresh after a moment.
    QTimer::singleShot(3000, this, &Network::refreshNetworks);
}

void Network::connectTo(const QString& ssid, const QString& password) {
    const auto device = wifiDevicePath();
    if (device.isEmpty())
        return;
    QDBusMessage msg;
    if (const auto known = m_knownBySsid.value(ssid); !known.isEmpty()) {
        msg = nmCall(kRootPath, kService, QStringLiteral("ActivateConnection"),
                     {QVariant::fromValue(QDBusObjectPath(known)), QVariant::fromValue(QDBusObjectPath(device)),
                      QVariant::fromValue(QDBusObjectPath(QStringLiteral("/")))});
    } else {
        NmSettings settings;
        settings[QStringLiteral("802-11-wireless")] = {{QStringLiteral("ssid"), ssid.toUtf8()}};
        if (!password.isEmpty())
            settings[QStringLiteral("802-11-wireless-security")] = {
                {QStringLiteral("key-mgmt"), QStringLiteral("wpa-psk")},
                {QStringLiteral("psk"), password}};
        const auto ap = m_apBySsid.value(ssid, QStringLiteral("/"));
        msg = nmCall(kRootPath, kService, QStringLiteral("AddAndActivateConnection"),
                     {QVariant::fromValue(settings), QVariant::fromValue(QDBusObjectPath(device)),
                      QVariant::fromValue(QDBusObjectPath(ap))});
    }
    m_connecting = true;
    m_lastError.clear();
    emit networksChanged();
    auto* watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
        w->deleteLater();
        m_connecting = false;
        if (w->isError())
            m_lastError = w->error().message();
        emit networksChanged();
        m_networksDebounce.start();
    });
}

void Network::disconnectWifi() {
    const auto device = wifiDevicePath();
    if (!device.isEmpty())
        QDBusConnection::systemBus().asyncCall(
            nmCall(device, kService + QLatin1String(".Device"), QStringLiteral("Disconnect")));
}

} // namespace island
