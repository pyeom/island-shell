#include "batteries.h"

#include "dbusproperties.h"
#include "dbusutil.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QJSEngine>

namespace island {

namespace {

const QString kService = QStringLiteral("org.freedesktop.UPower");
const QString kDeviceIface = QStringLiteral("org.freedesktop.UPower.Device");

// UPower device types we show, with a name for the UI.
QString kindOf(uint type) {
    switch (type) {
    case 5: return QStringLiteral("mouse");
    case 6: return QStringLiteral("keyboard");
    case 8: return QStringLiteral("phone");
    case 10: return QStringLiteral("tablet");
    case 12: return QStringLiteral("gamepad");
    case 13: return QStringLiteral("pen");
    case 14: return QStringLiteral("touchpad");
    case 17: return QStringLiteral("headset");
    case 18: return QStringLiteral("speakers");
    case 19: return QStringLiteral("headphones");
    case 22: return QStringLiteral("remote");
    case 26: return QStringLiteral("wearable");
    default: return QStringLiteral("other");
    }
}

QString iconFor(const QString& kind) {
    if (kind == u"mouse")
        return QStringLiteral("input-mouse");
    if (kind == u"keyboard")
        return QStringLiteral("input-keyboard");
    if (kind == u"gamepad")
        return QStringLiteral("input-gaming");
    if (kind == u"headset" || kind == u"headphones")
        return QStringLiteral("audio-headphones");
    if (kind == u"speakers")
        return QStringLiteral("audio-speakers");
    if (kind == u"phone")
        return QStringLiteral("phone");
    return QStringLiteral("battery");
}

constexpr uint kTypeBattery = 2;
constexpr uint kStateCharging = 1;
constexpr uint kStateFull = 4;
constexpr int kLow = 15;

} // namespace

Batteries* Batteries::create(QQmlEngine*, QJSEngine*) {
    static auto* b = new Batteries(QCoreApplication::instance());
    QJSEngine::setObjectOwnership(b, QJSEngine::CppOwnership);
    return b;
}

Batteries::Batteries(QObject* parent) : QObject(parent) {
    auto bus = QDBusConnection::systemBus();
    m_display = new DBusProperties(bus, kService, QStringLiteral("/org/freedesktop/UPower/devices/DisplayDevice"),
                                   kDeviceIface, this);
    connect(m_display, &DBusProperties::changed, this, &Batteries::changed);
    bus.connect(kService, QStringLiteral("/org/freedesktop/UPower"), kService, QStringLiteral("DeviceAdded"), this,
                SLOT(onDeviceAdded(QDBusMessage)));
    bus.connect(kService, QStringLiteral("/org/freedesktop/UPower"), kService, QStringLiteral("DeviceRemoved"),
                this, SLOT(onDeviceRemoved(QDBusMessage)));
    enumerate();
}

void Batteries::enumerate() {
    auto msg = QDBusMessage::createMethodCall(kService, QStringLiteral("/org/freedesktop/UPower"), kService,
                                              QStringLiteral("EnumerateDevices"));
    auto* watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
        w->deleteLater();
        const auto reply = w->reply();
        if (reply.type() != QDBusMessage::ReplyMessage)
            return;
        for (const auto& p : dbus::cast<QList<QDBusObjectPath>>(reply.arguments().value(0)))
            add(p.path());
    });
}

void Batteries::add(const QString& path) {
    if (m_devices.contains(path))
        return;
    auto* dev = new DBusProperties(QDBusConnection::systemBus(), kService, path, kDeviceIface, this);
    connect(dev, &DBusProperties::changed, this, &Batteries::changed);
    m_devices.insert(path, dev);
}

void Batteries::onDeviceAdded(const QDBusMessage& message) {
    add(dbus::toPath(message.arguments().value(0)));
}

void Batteries::onDeviceRemoved(const QDBusMessage& message) {
    delete m_devices.take(dbus::toPath(message.arguments().value(0)));
    emit changed();
}

bool Batteries::hasLaptopBattery() const {
    return m_display && m_display->ready() && m_display->value(QStringLiteral("IsPresent")).toBool() &&
           m_display->value(QStringLiteral("Type")).toUInt() == kTypeBattery;
}

int Batteries::laptopPercentage() const {
    return hasLaptopBattery() ? qRound(m_display->value(QStringLiteral("Percentage")).toDouble()) : -1;
}

bool Batteries::charging() const {
    const uint state = m_display ? m_display->value(QStringLiteral("State")).toUInt() : 0;
    return state == kStateCharging || state == kStateFull;
}

QVariantList Batteries::peripherals() const {
    QVariantList out;
    for (auto it = m_devices.cbegin(); it != m_devices.cend(); ++it) {
        const auto* d = it.value();
        const uint type = d->value(QStringLiteral("Type")).toUInt();
        // Not the laptop battery, line power or a UPS.
        if (d->value(QStringLiteral("PowerSupply")).toBool() || type <= 3)
            continue;
        const double percentage = d->value(QStringLiteral("Percentage")).toDouble();
        if (percentage <= 0)
            continue;
        const auto kind = kindOf(type);
        auto model = d->value(QStringLiteral("Model")).toString();
        if (model.isEmpty())
            model = kind;
        out.append(QVariantMap{{QStringLiteral("path"), it.key()},
                               {QStringLiteral("model"), model},
                               {QStringLiteral("kind"), kind},
                               {QStringLiteral("percentage"), qRound(percentage)},
                               {QStringLiteral("icon"), iconFor(kind)}});
    }
    return out;
}

QVariantMap Batteries::low() const {
    if (hasLaptopBattery() && !charging() && laptopPercentage() < kLow)
        return {{QStringLiteral("model"), QCoreApplication::translate("Batteries", "Battery")},
                {QStringLiteral("percentage"), laptopPercentage()},
                {QStringLiteral("icon"), QStringLiteral("battery-caution")}};
    for (const auto& v : peripherals()) {
        const auto p = v.toMap();
        if (p.value(QStringLiteral("percentage")).toInt() < kLow)
            return p;
    }
    return {};
}

} // namespace island
