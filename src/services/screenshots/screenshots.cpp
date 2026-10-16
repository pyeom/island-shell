#include "screenshots.h"

#include "apps.h"
#include "config.h"
#include "desktopentry.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJSEngine>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

Q_LOGGING_CATEGORY(lcShots, "island.screenshots")

namespace island {

Screenshots* Screenshots::instance() {
    static auto* s = new Screenshots(QCoreApplication::instance());
    return s;
}

Screenshots* Screenshots::create(QQmlEngine*, QJSEngine*) {
    auto* s = instance();
    QJSEngine::setObjectOwnership(s, QJSEngine::CppOwnership);
    return s;
}

Screenshots::Screenshots(QObject* parent) : QObject(parent) {}

QString Screenshots::directory() const {
    auto dir = QString::fromStdString(Config::instance()->data().screenshots.directory);
    if (dir.startsWith(QLatin1String("~/")))
        dir = QDir::homePath() + dir.mid(1);
    if (dir.isEmpty())
        dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) +
              QLatin1String("/Screenshots");
    return dir;
}

QString Screenshots::editor() const {
    const auto configured = QString::fromStdString(Config::instance()->data().screenshots.editor);
    if (!configured.isEmpty())
        return configured;
    if (!QStandardPaths::findExecutable(QStringLiteral("satty")).isEmpty())
        return QStringLiteral("satty --filename");
    if (!QStandardPaths::findExecutable(QStringLiteral("swappy")).isEmpty())
        return QStringLiteral("swappy -f");
    return {};
}

bool Screenshots::take(const QString& mode) {
    if (m_process)
        return false; // one at a time
    QStringList modes;
    if (mode == u"region")
        modes = {QStringLiteral("region")};
    else if (mode == u"window")
        modes = {QStringLiteral("window")};
    else if (mode == u"output")
        modes = {QStringLiteral("active"), QStringLiteral("output")};
    else if (mode == u"active")
        modes = {QStringLiteral("active"), QStringLiteral("window")};
    else
        return false;

    const auto dir = directory();
    QDir().mkpath(dir);
    const auto name = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss")) +
                      QLatin1String(".png");
    const auto path = dir + u'/' + name;

    QStringList args;
    for (const auto& m : modes)
        args << QStringLiteral("-m") << m;
    args << QStringLiteral("-o") << dir << QStringLiteral("-f") << name << QStringLiteral("--silent");

    m_process = new QProcess(this);
    connect(m_process, &QProcess::finished, this, [this, path] {
        m_process->deleteLater();
        m_process = nullptr;
        emit capturingChanged();
        // Cancelled selections leave no file.
        if (!QFile::exists(path))
            return;
        m_lastPath = path;
        qCInfo(lcShots).noquote() << "saved" << path;
        emit lastChanged();
        emit taken(path);
    });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        qCWarning(lcShots).noquote() << "hyprshot:" << m_process->errorString();
    });
    emit capturingChanged();
    // Let the island fade out before the screen is captured.
    QTimer::singleShot(200, m_process, [p = m_process, args] { p->start(QStringLiteral("hyprshot"), args); });
    return true;
}

void Screenshots::open() {
    if (!m_lastPath.isEmpty())
        Apps::instance()->openUrl(QUrl::fromLocalFile(m_lastPath).toString());
}

void Screenshots::copy() {
    if (m_lastPath.isEmpty())
        return;
    Apps::instance()->run(QLatin1String("wl-copy --type image/png < ") + shellQuote(m_lastPath));
}

void Screenshots::edit() {
    if (!m_lastPath.isEmpty() && canEdit())
        Apps::instance()->run(editor() + u' ' + shellQuote(m_lastPath));
}

void Screenshots::remove() {
    if (m_lastPath.isEmpty())
        return;
    QFile::moveToTrash(m_lastPath) || QFile::remove(m_lastPath);
    m_lastPath.clear();
    emit lastChanged();
}

void Screenshots::dismiss() {
    m_lastPath.clear();
    emit lastChanged();
}

} // namespace island
