#pragma once

#include <QDBusMessage>
#include <QDBusServiceWatcher>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

class DBusProperties;

// BlueZ: first adapter's power, its devices (with battery when the device
// reports one) and discovery. Copes with bluetoothd starting and stopping.
class Bluetooth : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(bool powered READ powered NOTIFY changed)
    Q_PROPERTY(bool discovering READ discovering NOTIFY changed)
    Q_PROPERTY(QStringList connectedDevices READ connectedDevices NOTIFY changed)
    // [{ path, name, icon, paired, connected, battery (-1: unknown) }],
    // connected first, then paired, then by name.
    Q_PROPERTY(QVariantList devices READ devices NOTIFY changed)

public:
    static Bluetooth* create(QQmlEngine*, QJSEngine*);

    bool available() const;
    bool powered() const;
    bool discovering() const;
    QStringList connectedDevices() const;
    QVariantList devices() const;

    Q_INVOKABLE void setPowered(bool powered);
    // Discovery while the device list is open.
    Q_INVOKABLE void setDiscovering(bool on);
    // Pairs first when needed (devices that need a PIN are not supported yet).
    Q_INVOKABLE void connectDevice(const QString& path);
    Q_INVOKABLE void disconnectDevice(const QString& path);

signals:
    void changed();

private slots:
    void onInterfacesAdded(const QDBusMessage& message);
    void onInterfacesRemoved(const QDBusMessage& message);

private:
    explicit Bluetooth(QObject* parent = nullptr);
    void load();
    void clear();
    void addObject(const QString& path, const QStringList& interfaces);
    void call(const QString& path, const QString& iface, const QString& method);

    struct Device {
        DBusProperties* device = nullptr;
        DBusProperties* battery = nullptr;
    };

    QDBusServiceWatcher m_watcher;
    QPointer<DBusProperties> m_adapter;
    QHash<QString, Device> m_devices;
    bool m_wantDiscovery = false; // applied once the adapter has loaded
};

} // namespace island
