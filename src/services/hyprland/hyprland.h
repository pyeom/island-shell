#pragma once

#include <QHash>
#include <QObject>
#include <QThread>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <functional>

class QQmlEngine;
class QJSEngine;

namespace island {

class HyprlandIpc;

// Hyprland IPC: requests over .socket.sock, events over .socket2.sock, both
// handled on a separate thread (see HyprlandIpc). Monitors and workspaces are
// re-read (debounced) after relevant events.
class Hyprland : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(QString focusedMonitor READ focusedMonitor NOTIFY focusedMonitorChanged)
    // [{ name, id, focused, activeWorkspace }]
    Q_PROPERTY(QVariantList monitors READ monitors NOTIFY monitorsChanged)
    // [{ id, name, monitor, windows }]
    Q_PROPERTY(QVariantList workspaces READ workspaces NOTIFY workspacesChanged)

public:
    static Hyprland* instance();
    static Hyprland* create(QQmlEngine*, QJSEngine*);
    ~Hyprland() override;

    bool available() const { return !m_socketDir.isEmpty(); }
    QString focusedMonitor() const { return m_focusedMonitor; }
    QVariantList monitors() const { return m_monitors; }
    QVariantList workspaces() const { return m_workspaces; }

    // hyprctl dispatch equivalent: Hyprland.dispatch("workspace 3")
    Q_INVOKABLE void dispatch(const QString& args);

    using Reply = std::function<void(const QByteArray&)>;
    void request(const QByteArray& command, Reply reply = {});

signals:
    void availableChanged();
    void focusedMonitorChanged();
    void monitorsChanged();
    void workspacesChanged();
    // Every event from the event socket, for services that need more.
    void event(const QString& name, const QString& data);

private:
    explicit Hyprland(QObject* parent = nullptr);
    void onEvent(const QString& name, const QString& data);
    void onReply(quint64 id, const QByteArray& data);
    void refresh();
    void setFocusedMonitor(const QString& name);

    QString m_socketDir;
    QThread m_thread;
    HyprlandIpc* m_ipc = nullptr;
    QHash<quint64, Reply> m_pending;
    quint64 m_nextRequest = 1;
    QTimer m_refresh;
    QString m_focusedMonitor;
    QVariantList m_monitors;
    QVariantList m_workspaces;
};

} // namespace island
