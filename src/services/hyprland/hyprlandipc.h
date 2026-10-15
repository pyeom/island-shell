#pragma once

#include <QByteArray>
#include <QLocalSocket>
#include <QObject>
#include <QTimer>

namespace island {

// Hyprland's sockets, driven from a dedicated thread.
//
// Hyprland serves .socket.sock synchronously: after accepting a connection
// it blocks until the request arrives. If the request were written from the
// GUI thread while that thread waits on the compositor (EGL setup does a
// Wayland roundtrip when a window is created), both sides would wait on each
// other until Hyprland's IPC timeout. Here nothing the GUI thread does can
// delay a request.
class HyprlandIpc : public QObject {
    Q_OBJECT

public:
    explicit HyprlandIpc(const QString& socketDir);

public slots:
    void start();
    // Blocking within this thread: connect, write, read until closed.
    void request(quint64 id, const QByteArray& command);

signals:
    void reply(quint64 id, const QByteArray& data);
    void event(const QString& name, const QString& data);
    void eventsConnected();

private:
    void connectEvents();
    void onEventData();

    QString m_socketDir;
    QLocalSocket* m_events = nullptr;
    QTimer* m_reconnect = nullptr;
    QByteArray m_buffer;
};

} // namespace island
