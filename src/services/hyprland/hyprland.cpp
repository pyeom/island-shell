#include "hyprland.h"

#include "hyprlandparse.h"

#include <QCoreApplication>
#include <QDir>
#include <QJSEngine>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcHypr, "island.hyprland")

namespace island {

Hyprland* Hyprland::instance() {
    static auto* hypr = new Hyprland(QCoreApplication::instance());
    return hypr;
}

Hyprland* Hyprland::create(QQmlEngine*, QJSEngine*) {
    auto* hypr = instance();
    QJSEngine::setObjectOwnership(hypr, QJSEngine::CppOwnership);
    return hypr;
}

Hyprland::Hyprland(QObject* parent) : QObject(parent) {
    const auto signature = qEnvironmentVariable("HYPRLAND_INSTANCE_SIGNATURE");
    if (signature.isEmpty()) {
        qCInfo(lcHypr) << "HYPRLAND_INSTANCE_SIGNATURE not set; Hyprland integration disabled";
        return;
    }
    auto runtime = qEnvironmentVariable("XDG_RUNTIME_DIR");
    if (runtime.isEmpty())
        runtime = QStringLiteral("/tmp");
    m_socketDir = runtime + QLatin1String("/hypr/") + signature;

    m_refresh.setSingleShot(true);
    m_refresh.setInterval(25);
    connect(&m_refresh, &QTimer::timeout, this, &Hyprland::refresh);

    m_reconnect.setSingleShot(true);
    m_reconnect.setInterval(1000);
    connect(&m_reconnect, &QTimer::timeout, this, &Hyprland::connectEvents);

    connect(&m_events, &QLocalSocket::readyRead, this, &Hyprland::onEventData);
    connect(&m_events, &QLocalSocket::connected, this, &Hyprland::refresh);
    connect(&m_events, &QLocalSocket::disconnected, &m_reconnect, qOverload<>(&QTimer::start));
    connect(&m_events, &QLocalSocket::errorOccurred, this, [this](QLocalSocket::LocalSocketError) {
        qCWarning(lcHypr) << "event socket:" << m_events.errorString();
        m_reconnect.start();
    });

    connectEvents();
}

void Hyprland::connectEvents() {
    m_eventBuffer.clear();
    m_events.abort();
    m_events.connectToServer(m_socketDir + QLatin1String("/.socket2.sock"));
}

void Hyprland::onEventData() {
    m_eventBuffer += m_events.readAll();
    qsizetype start = 0;
    for (qsizetype nl; (nl = m_eventBuffer.indexOf('\n', start)) >= 0; start = nl + 1) {
        const auto ev = hypr::parseEventLine(QByteArrayView(m_eventBuffer).sliced(start, nl - start));
        if (!ev)
            continue;
        if (ev->name == QLatin1String("focusedmon"))
            setFocusedMonitor(ev->data.section(QLatin1Char(','), 0, 0));
        if (hypr::affectsWorkspaces(ev->name))
            m_refresh.start();
        emit event(ev->name, ev->data);
    }
    m_eventBuffer.remove(0, start);
}

void Hyprland::request(const QByteArray& command, Reply reply) {
    if (!available())
        return;
    auto* socket = new QLocalSocket(this);
    auto buffer = std::make_shared<QByteArray>();
    connect(socket, &QLocalSocket::connected, socket, [socket, command] { socket->write(command); });
    connect(socket, &QLocalSocket::readyRead, socket,
            [socket, buffer] { *buffer += socket->readAll(); });
    connect(socket, &QLocalSocket::disconnected, socket, [socket, buffer, reply] {
        *buffer += socket->readAll();
        if (reply)
            reply(*buffer);
        socket->deleteLater();
    });
    connect(socket, &QLocalSocket::errorOccurred, socket, [socket](QLocalSocket::LocalSocketError e) {
        if (e != QLocalSocket::PeerClosedError) {
            qCWarning(lcHypr) << "request failed:" << socket->errorString();
            socket->deleteLater();
        }
    });
    socket->connectToServer(m_socketDir + QLatin1String("/.socket.sock"));
}

void Hyprland::dispatch(const QString& args) {
    request("dispatch " + args.toUtf8(), [args](const QByteArray& reply) {
        if (reply.trimmed() != "ok")
            qCWarning(lcHypr).noquote() << "dispatch" << args << "->" << reply.trimmed();
    });
}

void Hyprland::refresh() {
    request("j/monitors", [this](const QByteArray& json) {
        QString focused;
        auto monitors = hypr::parseMonitors(json, &focused);
        if (monitors != m_monitors) {
            m_monitors = std::move(monitors);
            emit monitorsChanged();
        }
        if (!focused.isEmpty())
            setFocusedMonitor(focused);
    });
    request("j/workspaces", [this](const QByteArray& json) {
        auto workspaces = hypr::parseWorkspaces(json);
        if (workspaces != m_workspaces) {
            m_workspaces = std::move(workspaces);
            emit workspacesChanged();
        }
    });
}

void Hyprland::setFocusedMonitor(const QString& name) {
    if (m_focusedMonitor == name)
        return;
    m_focusedMonitor = name;
    emit focusedMonitorChanged();
}

} // namespace island
