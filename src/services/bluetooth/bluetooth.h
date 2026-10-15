#pragma once

#include <QDBusObjectPath>
#include <QDBusServiceWatcher>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

class DBusProperties;

using InterfaceMap = QMap<QString, QVariantMap>;
using ManagedObjects = QMap<QDBusObjectPath, InterfaceMap>;

// BlueZ: first adapter's power state and connected devices. Copes with
// bluetoothd starting and stopping.
class Bluetooth : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(bool powered READ powered NOTIFY changed)
    Q_PROPERTY(QStringList connectedDevices READ connectedDevices NOTIFY changed)

public:
    static Bluetooth* create(QQmlEngine*, QJSEngine*);

    bool available() const;
    bool powered() const;
    QStringList connectedDevices() const;

    Q_INVOKABLE void setPowered(bool powered);

signals:
    void changed();

private slots:
    void onInterfacesAdded(const QDBusObjectPath& path, const InterfaceMap& interfaces);
    void onInterfacesRemoved(const QDBusObjectPath& path, const QStringList& interfaces);

private:
    explicit Bluetooth(QObject* parent = nullptr);
    void load();
    void clear();
    void addObject(const QString& path, const InterfaceMap& interfaces);

    QDBusServiceWatcher m_watcher;
    QPointer<DBusProperties> m_adapter;
    QHash<QString, DBusProperties*> m_devices;
};

} // namespace island

Q_DECLARE_METATYPE(island::InterfaceMap)
Q_DECLARE_METATYPE(island::ManagedObjects)
