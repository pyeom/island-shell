#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantList>
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
    // Visible Wi-Fi networks, strongest per SSID: [{ ssid, strength, secured,
    // known, active }], sorted active first, then by strength.
    Q_PROPERTY(QVariantList networks READ networks NOTIFY networksChanged)
    Q_PROPERTY(bool connecting READ connecting NOTIFY networksChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY networksChanged)

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

    QVariantList networks() const { return m_networks; }
    bool connecting() const { return m_connecting; }
    QString lastError() const { return m_lastError; }

    // Asks the Wi-Fi device to rescan; `networks` updates when it finishes.
    Q_INVOKABLE void scan();
    // Known networks reconnect with their saved profile; new ones are added
    // (with `password` for secured networks).
    Q_INVOKABLE void connectTo(const QString& ssid, const QString& password = {});
    Q_INVOKABLE void disconnectWifi();
    // The network list refreshes only while someone looks at it.
    Q_INVOKABLE void setWatching(bool watching);

signals:
    void changed();
    void networksChanged();

private:
    explicit Network(QObject* parent = nullptr);
    void sync();
    void syncDevices();
    void syncActive();
    void syncWireless();
    void refreshNetworks();
    QString wifiDevicePath() const;

    DBusProperties* m_root = nullptr;
    QPointer<DBusProperties> m_active;
    QPointer<DBusProperties> m_wireless;
    QPointer<DBusProperties> m_ap;
    QHash<QString, DBusProperties*> m_devices;
    bool m_hasWifi = false;
    QString m_ssid;
    int m_strength = 0;
    QTimer m_debounce;
    QTimer m_networksDebounce;
    QVariantList m_networks;
    QHash<QString, QString> m_apBySsid;      // ssid -> strongest access point
    QHash<QString, QString> m_knownBySsid;   // ssid -> saved connection
    bool m_connecting = false;
    bool m_watching = false;
    QString m_lastError;
};

} // namespace island
