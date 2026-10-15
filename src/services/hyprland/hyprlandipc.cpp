#include "hyprlandipc.h"

#include "hyprlandparse.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcHypr)

namespace island {

namespace {
constexpr int kTimeoutMs = 2000;
}

HyprlandIpc::HyprlandIpc(const QString& socketDir) : m_socketDir(socketDir) {}

void HyprlandIpc::start() {
    // Created here so they belong to the IPC thread.
    m_events = new QLocalSocket(this);
    m_reconnect = new QTimer(this);
    m_reconnect->setSingleShot(true);
    m_reconnect->setInterval(1000);
    connect(m_reconnect, &QTimer::timeout, this, &HyprlandIpc::connectEvents);
    connect(m_events, &QLocalSocket::readyRead, this, &HyprlandIpc::onEventData);
    connect(m_events, &QLocalSocket::connected, this, &HyprlandIpc::eventsConnected);
    connect(m_events, &QLocalSocket::disconnected, m_reconnect, qOverload<>(&QTimer::start));
    connect(m_events, &QLocalSocket::errorOccurred, this, [this](QLocalSocket::LocalSocketError) {
        qCWarning(lcHypr) << "event socket:" << m_events->errorString();
        m_reconnect->start();
    });
    connectEvents();
}

void HyprlandIpc::connectEvents() {
    m_buffer.clear();
    m_events->abort();
    m_events->connectToServer(m_socketDir + QLatin1String("/.socket2.sock"));
}

void HyprlandIpc::onEventData() {
    m_buffer += m_events->readAll();
    qsizetype start = 0;
    for (qsizetype nl; (nl = m_buffer.indexOf('\n', start)) >= 0; start = nl + 1) {
        if (const auto ev = hypr::parseEventLine(QByteArrayView(m_buffer).sliced(start, nl - start)))
            emit event(ev->name, ev->data);
    }
    m_buffer.remove(0, start);
}

void HyprlandIpc::request(quint64 id, const QByteArray& command) {
    QLocalSocket socket;
    socket.connectToServer(m_socketDir + QLatin1String("/.socket.sock"));
    QByteArray data;
    if (socket.waitForConnected(kTimeoutMs)) {
        socket.write(command);
        socket.waitForBytesWritten(kTimeoutMs);
        // Hyprland closes the connection after the reply.
        while (socket.state() == QLocalSocket::ConnectedState &&
               socket.waitForReadyRead(kTimeoutMs))
            data += socket.readAll();
        data += socket.readAll();
    } else {
        qCWarning(lcHypr) << "request failed:" << socket.errorString();
    }
    emit reply(id, data);
}

} // namespace island
