#include "nightlight.h"

#include "apps.h"
#include "config.h"
#include "desktopentry.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
#include <QStandardPaths>

#include <csignal>

namespace island {

namespace {
QByteArray readFile(const QString& path) {
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}
} // namespace

NightLight* NightLight::create(QQmlEngine*, QJSEngine*) {
    static auto* n = new NightLight(QCoreApplication::instance());
    QJSEngine::setObjectOwnership(n, QJSEngine::CppOwnership);
    return n;
}

NightLight::NightLight(QObject* parent) : QObject(parent) {
    // wlsunset can be started or killed outside the island too.
    m_poll.setInterval(3000);
    connect(&m_poll, &QTimer::timeout, this, &NightLight::poll);
    m_poll.start();
    scan();
}

bool NightLight::available() const {
    return !Config::instance()->data().nightLight.command.empty() ||
           !QStandardPaths::findExecutable(QStringLiteral("wlsunset")).isEmpty();
}

QString NightLight::statePath() const {
    return QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation) +
           QLatin1String("/island-shell/nightlight-command");
}

void NightLight::poll() {
    // The wlsunset we know about: one read instead of all of /proc.
    if (m_pid > 0 && readFile(QStringLiteral("/proc/%1/comm").arg(m_pid)).trimmed() == "wlsunset")
        return;
    // Not running: started from elsewhere is rarer, look every 30 s.
    if (m_pid > 0 || ++m_idlePolls >= 10) {
        m_idlePolls = 0;
        scan();
    }
}

void NightLight::scan() {
    qint64 found = 0;
    const QDir proc(QStringLiteral("/proc"));
    for (const auto& pid : proc.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (pid.front().isDigit() &&
            readFile(QStringLiteral("/proc/%1/comm").arg(pid)).trimmed() == "wlsunset") {
            found = pid.toLongLong();
            break;
        }
    }
    if (found != m_pid) {
        m_pid = found;
        emit changed();
    }
}

QString NightLight::command() const {
    const auto configured = QString::fromStdString(Config::instance()->data().nightLight.command);
    if (!configured.isEmpty())
        return configured;
    const auto saved = QString::fromUtf8(readFile(statePath())).trimmed();
    return saved.isEmpty() ? QStringLiteral("wlsunset") : saved;
}

void NightLight::setRunning(bool on) {
    scan();
    if (on == running())
        return;
    if (on) {
        Apps::instance()->run(command());
    } else {
        // Remember how it was started: wlsunset's arguments carry the
        // location and temperatures.
        const auto args = readFile(QStringLiteral("/proc/%1/cmdline").arg(m_pid)).split('\0');
        QStringList quoted;
        for (const auto& a : args)
            if (!a.isEmpty())
                quoted.append(shellQuote(QString::fromUtf8(a)));
        if (!quoted.isEmpty()) {
            QDir().mkpath(QFileInfo(statePath()).path());
            QFile f(statePath());
            if (f.open(QIODevice::WriteOnly))
                f.write(quoted.join(u' ').toUtf8());
        }
        ::kill(static_cast<pid_t>(m_pid), SIGTERM);
    }
    QTimer::singleShot(500, this, &NightLight::scan);
}

} // namespace island
