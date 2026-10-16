#pragma once

#include <QDBusMessage>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

class DBusProperties;

// Batteries from UPower: the laptop's (display device) and peripherals'
// (headsets, controllers, mice... anything that is not a power supply).
class Batteries : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool hasLaptopBattery READ hasLaptopBattery NOTIFY changed)
    Q_PROPERTY(int laptopPercentage READ laptopPercentage NOTIFY changed)
    Q_PROPERTY(bool charging READ charging NOTIFY changed)
    // [{ path, model, kind, percentage, icon }]
    Q_PROPERTY(QVariantList peripherals READ peripherals NOTIFY changed)
    // { model, percentage, icon } of the first battery under 15% (laptop:
    // only while discharging), else {}.
    Q_PROPERTY(QVariantMap low READ low NOTIFY changed)

public:
    static Batteries* create(QQmlEngine*, QJSEngine*);

    bool hasLaptopBattery() const;
    int laptopPercentage() const;
    bool charging() const;
    QVariantList peripherals() const;
    QVariantMap low() const;

signals:
    void changed();

private slots:
    void onDeviceAdded(const QDBusMessage& message);
    void onDeviceRemoved(const QDBusMessage& message);

private:
    explicit Batteries(QObject* parent = nullptr);
    void enumerate();
    void add(const QString& path);

    QPointer<DBusProperties> m_display;
    QHash<QString, DBusProperties*> m_devices;
};

} // namespace island
