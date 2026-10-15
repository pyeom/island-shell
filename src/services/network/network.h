#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

class DBusProperties;

// NetworkManager summary: primary connection, Wi-Fi radio and signal.
class Network : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(bool connected READ connected NOTIFY changed)
    // "wifi" | "ethernet" | "vpn" | "other" | "" (none)
    Q_PROPERTY(QString primaryType READ primaryType NOTIFY changed)
    Q_PROPERTY(QString connectionName READ connectionName NOTIFY changed)
    Q_PROPERTY(bool hasWifi READ hasWifi NOTIFY changed)
    Q_PROPERTY(bool wifiEnabled READ wifiEnabled NOTIFY changed)
    Q_PROPERTY(QString ssid READ ssid NOTIFY changed)
    Q_PROPERTY(int strength READ strength NOTIFY changed) // 0..100
    Q_PROPERTY(QString iconName READ iconName NOTIFY changed)

public:
    static Network* create(QQmlEngine*, QJSEngine*);

    bool available() const;
    bool connected() const;
    QString primaryType() const;
    QString connectionName() const;
    bool hasWifi() const { return m_hasWifi; }
    bool wifiEnabled() const;
    QString ssid() const { return m_ssid; }
    int strength() const { return m_strength; }
    QString iconName() const;

    Q_INVOKABLE void setWifiEnabled(bool enabled);

signals:
    void changed();

private:
    explicit Network(QObject* parent = nullptr);
    void sync();
    void syncDevices();
    void syncActive();
    void syncWireless();

    DBusProperties* m_root = nullptr;
    QPointer<DBusProperties> m_active;
    QPointer<DBusProperties> m_wireless;
    QPointer<DBusProperties> m_ap;
    QHash<QString, DBusProperties*> m_devices;
    bool m_hasWifi = false;
    QString m_ssid;
    int m_strength = 0;
    QTimer m_debounce;
};

} // namespace island
