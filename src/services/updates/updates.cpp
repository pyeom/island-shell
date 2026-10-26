#include "updates.h"

#include "apps.h"
#include "config.h"

#include <QCoreApplication>
#include <QJSEngine>
#include <QLoggingCategory>
#include <QProcess>
#include <QStandardPaths>

#include <utility>

Q_LOGGING_CATEGORY(lcUpdates, "island.updates")

namespace island {

namespace {
const QString kLocalDb = QStringLiteral("/var/lib/pacman/local");

bool installed(const QString& program) {
    return !QStandardPaths::findExecutable(program).isEmpty();
}
} // namespace

Updates* Updates::instance() {
    static auto* u = new Updates(QCoreApplication::instance());
    return u;
}

Updates* Updates::create(QQmlEngine*, QJSEngine*) {
    auto* u = instance();
    QJSEngine::setObjectOwnership(u, QJSEngine::CppOwnership);
    return u;
}

Updates::Updates(QObject* parent) : QObject(parent) {
    if (!available())
        return;
    connect(&m_timer, &QTimer::timeout, this, &Updates::check);
    connect(Config::instance(), &Config::changed, this, [this] {
        applyInterval();
        // Another manager counts the AUR or not: count again.
        if (std::exchange(m_manager, manager()) != m_manager && m_checked)
            check();
    });
    m_manager = manager();
    applyInterval();

    // pacman writes many files per package: check once it has settled.
    m_dbChanged.setSingleShot(true);
    m_dbChanged.setInterval(10 * 1000);
    connect(&m_dbChanged, &QTimer::timeout, this, &Updates::check);
    m_watcher.addPath(kLocalDb);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this,
            [this] { m_dbChanged.start(); });

    // Not at startup: the network may not be up yet.
    QTimer::singleShot(60 * 1000, this, &Updates::check);
}

bool Updates::available() const {
    return installed(QStringLiteral("checkupdates"));
}

void Updates::applyInterval() {
    const int minutes = Config::instance()->data().updates.interval;
    if (minutes <= 0) {
        m_timer.stop();
        return;
    }
    if (m_timer.interval() != minutes * 60 * 1000 || !m_timer.isActive())
        m_timer.start(minutes * 60 * 1000);
}

QString Updates::manager() const {
    const auto chosen = QString::fromStdString(Config::instance()->data().updates.manager);
    if (chosen != u"auto")
        // A helper that is not installed falls back to pacman.
        return chosen == u"pacman" || installed(chosen) ? chosen : QStringLiteral("pacman");
    for (const auto* helper : {"paru", "yay"})
        if (installed(QLatin1String(helper)))
            return QLatin1String(helper);
    return QStringLiteral("pacman");
}

QString Updates::aurHelper() const {
    const auto m = manager();
    return m == u"pacman" ? QString() : m;
}

void Updates::check() {
    if (!available())
        return;
    if (checking()) {
        m_recheck = true;
        return;
    }
    m_found.clear();
    m_aurCount = 0;
    runStep(Step::Repo);
    emit changed();
}

void Updates::runStep(Step step) {
    m_step = step;
    auto* p = new QProcess(this);
    connect(p, &QProcess::finished, this, [this, p, step](int code, QProcess::ExitStatus status) {
        p->deleteLater();
        const auto out = QString::fromUtf8(p->readAllStandardOutput());
        const auto lines = out.split(u'\n', Qt::SkipEmptyParts);
        // checkupdates: 0 updates, 2 none, anything else failed. paru/yay
        // -Qua: 0 updates, 1 none.
        const bool ok = status == QProcess::NormalExit &&
                        (code == 0 || (step == Step::Repo ? code == 2 : code == 1));
        if (!ok) {
            qCWarning(lcUpdates).noquote()
                << (step == Step::Repo ? "checkupdates" : "AUR check") << "failed:"
                << QString::fromUtf8(p->readAllStandardError()).trimmed();
        }
        if (step == Step::Repo) {
            m_failed = !ok;
            if (ok)
                m_found = lines;
            if (ok && !aurHelper().isEmpty()) {
                runStep(Step::Aur);
                return;
            }
        } else if (ok) {
            // "name old -> new" like checkupdates; ignored packages say so.
            for (const auto& line : lines) {
                if (line.contains(QLatin1String("[ignored]")))
                    continue;
                m_found.append(line);
                ++m_aurCount;
            }
        }
        m_step = Step::Idle;
        m_checked = true;
        if (!m_failed)
            m_packages = m_found;
        qCInfo(lcUpdates) << m_packages.size() << "updates," << m_aurCount << "from the AUR";
        emit changed();
        if (std::exchange(m_recheck, false))
            check();
    });
    if (step == Step::Repo) {
        // --nocolor: no escape codes to strip.
        p->start(QStringLiteral("checkupdates"), {QStringLiteral("--nocolor")});
    } else {
        p->start(aurHelper(), {QStringLiteral("-Qua")});
    }
}

void Updates::upgrade() {
    auto command = QString::fromStdString(Config::instance()->data().updates.command).trimmed();
    if (command.isEmpty()) {
        const auto helper = aurHelper();
        command = helper.isEmpty() ? QStringLiteral("sudo pacman -Syu") : helper + QLatin1String(" -Syu");
    }
    if (installed(QStringLiteral("island-run")))
        command = QLatin1String("island-run ") + command;
    Apps::instance()->runInTerminal(command);
}

} // namespace island
