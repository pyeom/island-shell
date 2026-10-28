#pragma once

#include <QDBusContext>
#include <QDBusServiceWatcher>
#include <QObject>
#include <QStringList>

namespace island {

// org.kde.StatusNotifierWatcher, for sessions where nothing else (waybar, a
// panel) keeps the list of tray items. Apps register with it; Tray reads it.
class StatusNotifierWatcher : public QObject, protected QDBusContext {
    Q_OBJECT

    // D-Bus properties (names fixed by the interface).
    Q_PROPERTY(QStringList RegisteredStatusNotifierItems READ items)
    Q_PROPERTY(bool IsStatusNotifierHostRegistered READ hostRegistered)
    Q_PROPERTY(int ProtocolVersion READ protocolVersion)

public:
    static constexpr auto kService = "org.kde.StatusNotifierWatcher";
    static constexpr auto kPath = "/StatusNotifierWatcher";

    explicit StatusNotifierWatcher(QObject* parent = nullptr);
    ~StatusNotifierWatcher() override;

    // Takes the bus name; false when another watcher owns it.
    bool registerOnBus();

    QStringList items() const { return m_items; }
    bool hostRegistered() const { return !m_hosts.isEmpty(); }
    int protocolVersion() const { return 0; }

    // The island's own host, which does not go through the bus.
    void addLocalHost();

public slots:
    void RegisterStatusNotifierItem(const QString& service);
    void RegisterStatusNotifierHost(const QString& service);

signals:
    void StatusNotifierItemRegistered(const QString& service);
    void StatusNotifierItemUnregistered(const QString& service);
    void StatusNotifierHostRegistered();
    void StatusNotifierHostUnregistered();

private:
    void onServiceGone(const QString& service);

    QStringList m_items; // "<bus name><object path>"
    QStringList m_hosts;
    QDBusServiceWatcher m_watcher;
    bool m_registered = false;
};

} // namespace island
