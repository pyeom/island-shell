#include "hyprland.h"

#include "hyprlandipc.h"
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

    m_ipc = new HyprlandIpc(m_socketDir);
    m_ipc->moveToThread(&m_thread);
    connect(&m_thread, &QThread::started, m_ipc, &HyprlandIpc::start);
    connect(&m_thread, &QThread::finished, m_ipc, &QObject::deleteLater);
    connect(m_ipc, &HyprlandIpc::event, this, &Hyprland::onEvent);
    connect(m_ipc, &HyprlandIpc::reply, this, &Hyprland::onReply);
    connect(m_ipc, &HyprlandIpc::eventsConnected, this, &Hyprland::refresh);
    m_thread.setObjectName(QStringLiteral("HyprlandIpc"));
    m_thread.start();
}

Hyprland::~Hyprland() {
    m_thread.quit();
    m_thread.wait();
}

void Hyprland::onEvent(const QString& name, const QString& data) {
    if (name == QLatin1String("focusedmon"))
        setFocusedMonitor(data.section(QLatin1Char(','), 0, 0));
    if (hypr::affectsWorkspaces(name))
        m_refresh.start();
    emit event(name, data);
}

void Hyprland::request(const QByteArray& command, Reply reply) {
    if (!available())
        return;
    const quint64 id = m_nextRequest++;
    if (reply)
        m_pending.insert(id, std::move(reply));
    QMetaObject::invokeMethod(m_ipc, "request", Qt::QueuedConnection, Q_ARG(quint64, id),
                              Q_ARG(QByteArray, command));
}

void Hyprland::onReply(quint64 id, const QByteArray& data) {
    const auto reply = m_pending.take(id);
    if (reply)
        reply(data);
}

void Hyprland::dispatch(const QString& args) {
    request("dispatch " + args.toUtf8(), [args](const QByteArray& reply) {
        if (reply.trimmed() != "ok")
            qCWarning(lcHypr).noquote() << "dispatch" << args << "->" << reply.trimmed();
    });
}

void Hyprland::send(const hypr::Dispatch& d) {
    if (m_syntax == Syntax::Lua) {
        dispatch(d.lua);
        return;
    }
    request("dispatch " + d.legacy.toUtf8(), [this, d](const QByteArray& reply) {
        if (reply.trimmed() == "ok") {
            m_syntax = Syntax::Conf;
        } else if (hypr::isLuaSyntaxError(reply)) {
            qCInfo(lcHypr) << "Lua config: using hl.dsp dispatchers";
            m_syntax = Syntax::Lua;
            dispatch(d.lua);
        } else {
            qCWarning(lcHypr).noquote() << "dispatch" << d.legacy << "->" << reply.trimmed();
        }
    });
}

void Hyprland::exec(const QString& command) {
    send(hypr::execDispatch(command));
}

void Hyprland::focusWorkspace(int id) {
    send(hypr::workspaceDispatch(id));
}

void Hyprland::focusWindow(const QString& address) {
    send(hypr::focusWindowDispatch(address));
}

void Hyprland::exit() {
    send(hypr::exitDispatch());
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
    // Clients tell a fullscreen window from a maximized one.
    request("j/workspaces", [this](const QByteArray& json) {
        request("j/clients", [this, json](const QByteArray& clients) {
            auto workspaces = hypr::parseWorkspaces(json, clients);
            if (workspaces != m_workspaces) {
                m_workspaces = std::move(workspaces);
                emit workspacesChanged();
            }
        });
    });
}

void Hyprland::setFocusedMonitor(const QString& name) {
    if (m_focusedMonitor == name)
        return;
    m_focusedMonitor = name;
    emit focusedMonitorChanged();
}

} // namespace island
