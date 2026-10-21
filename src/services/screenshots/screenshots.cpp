#include "screenshots.h"

#include "apps.h"
#include "config.h"
#include "desktopentry.h"
#include "hyprland.h"
#include "shotgeometry.h"

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

// $1 mode, $2 output file, $3 geometry or output name. slurp exits non-zero
// when the selection is cancelled.
static const char kCapture[] = R"(case $1 in
region) g=$(slurp) || exit 1 ;;
window) g=$(slurp -r) || exit 1 ;;
output) exec grim -o "$3" "$2" ;;
*) g=$3 ;;
esac
exec grim -g "$g" "$2"
)";

bool Screenshots::take(const QString& mode) {
    if (m_process)
        return false; // one at a time
    if (mode != u"region" && mode != u"window" && mode != u"output" && mode != u"active")
        return false;
    auto* hypr = Hyprland::instance();
    if (mode != u"region" && !hypr->available())
        return false;

    const auto dir = directory();
    QDir().mkpath(dir);
    const auto path = dir + u'/' +
                      QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss")) +
                      QLatin1String(".png");

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
    const auto done = [this, p = m_process] {
        if (m_process != p)
            return;
        p->deleteLater();
        m_process = nullptr;
        emit capturingChanged();
    };
    connect(m_process, &QProcess::finished, this, [this, path, done] {
        done();
        // Cancelled selections leave no file.
        if (!QFile::exists(path))
            return;
        m_lastPath = path;
        qCInfo(lcShots).noquote() << "saved" << path;
        emit lastChanged();
        emit taken(path);
        copy();
    });
    connect(m_process, &QProcess::errorOccurred, this, [p = m_process, done](QProcess::ProcessError e) {
        qCWarning(lcShots).noquote() << "capture:" << p->errorString();
        if (e == QProcess::FailedToStart)
            done(); // no finished() follows
    });
    emit capturingChanged();

    // Let the island fade out before the screen is captured.
    const auto start = [p = m_process, mode, path](const QString& arg, const QByteArray& input = {}) {
        QTimer::singleShot(200, p, [p, mode, path, arg, input] {
            p->start(QStringLiteral("sh"), {QStringLiteral("-c"), QLatin1String(kCapture),
                                            QStringLiteral("sh"), mode, path, arg});
            p->write(input);
            p->closeWriteChannel();
        });
    };
    if (mode == u"region") {
        start({});
    } else if (mode == u"output") {
        start(hypr->focusedMonitor());
    } else if (mode == u"window") {
        hypr->request("j/clients", [hypr, start](const QByteArray& clients) {
            hypr->request("j/monitors", [clients, start](const QByteArray& monitors) {
                start({}, shots::windowBoxes(clients, monitors).toUtf8());
            });
        });
    } else {
        hypr->request("j/activewindow", [start, done](const QByteArray& json) {
            if (const auto box = shots::activeWindowBox(json))
                start(*box);
            else
                done(); // no focused window
        });
    }
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
