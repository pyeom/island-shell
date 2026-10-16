#include "wallpapers.h"

#include "apps.h"
#include "config.h"
#include "desktopentry.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJSEngine>
#include <QRandomGenerator>
#include <QStandardPaths>

namespace island {

namespace {
bool has(const char* program) {
    return !QStandardPaths::findExecutable(QLatin1String(program)).isEmpty();
}
} // namespace

Wallpapers* Wallpapers::create(QQmlEngine*, QJSEngine*) {
    static auto* w = new Wallpapers(QCoreApplication::instance());
    QJSEngine::setObjectOwnership(w, QJSEngine::CppOwnership);
    return w;
}

Wallpapers::Wallpapers(QObject* parent) : QObject(parent) {
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &Wallpapers::scan);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &Wallpapers::scan);
    connect(Config::instance(), &Config::changed, this, &Wallpapers::scan);
    scan();
}

QString Wallpapers::directory() const {
    auto dir = QString::fromStdString(Config::instance()->data().wallpaper.directory);
    if (dir.startsWith(QLatin1String("~/")))
        dir = QDir::homePath() + dir.mid(1);
    return dir;
}

void Wallpapers::scan() {
    const QDir dir(directory());
    QStringList files;
    for (const auto& info : dir.entryInfoList({QStringLiteral("*.png"), QStringLiteral("*.jpg"),
                                               QStringLiteral("*.jpeg"), QStringLiteral("*.webp"),
                                               QStringLiteral("*.gif")},
                                              QDir::Files, QDir::Name | QDir::IgnoreCase))
        files.append(info.absoluteFilePath());

    // Current wallpaper: what pywal last applied.
    const auto walCache = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) +
                          QLatin1String("/wal/wal");
    QFile f(walCache);
    const auto current = f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()).trimmed() : QString();

    if (!m_watcher.directories().isEmpty())
        m_watcher.removePaths(m_watcher.directories());
    if (!m_watcher.files().isEmpty())
        m_watcher.removePaths(m_watcher.files());
    if (dir.exists())
        m_watcher.addPath(dir.absolutePath());
    if (QFile::exists(walCache))
        m_watcher.addPath(walCache);

    if (files != m_files || current != m_current) {
        m_files = files;
        m_current = current;
        emit changed();
    }
}

QString Wallpapers::applyCommand() const {
    const auto configured = QString::fromStdString(Config::instance()->data().wallpaper.apply);
    if (!configured.isEmpty())
        return configured;
    QStringList steps;
    if (has("waypaper"))
        steps << QStringLiteral("waypaper --wallpaper %f");
    else if (has("awww"))
        steps << QStringLiteral("awww img %f");
    else if (has("swww"))
        steps << QStringLiteral("swww img %f");
    else
        steps << QStringLiteral("hyprctl hyprpaper reload ,%f");
    if (has("wal"))
        steps << QStringLiteral("wal -q -n -i %f");
    return steps.join(QLatin1String(" && "));
}

void Wallpapers::apply(const QString& file) {
    if (!m_files.contains(file))
        return;
    auto command = applyCommand();
    command.replace(QStringLiteral("%f"), shellQuote(file));
    Apps::instance()->run(command);
}

void Wallpapers::random() {
    QStringList others = m_files;
    others.removeAll(m_current);
    if (others.isEmpty())
        others = m_files;
    if (!others.isEmpty())
        apply(others.at(static_cast<qsizetype>(QRandomGenerator::global()->bounded(others.size()))));
}

} // namespace island
