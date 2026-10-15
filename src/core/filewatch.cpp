#include "filewatch.h"

#include <QDir>
#include <QFileInfo>

namespace island {

FileWatch::FileWatch(QObject* parent) : QObject(parent) {
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(150);
    connect(&m_debounce, &QTimer::timeout, this, [this] {
        rewatch();
        emit changed();
    });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, &m_debounce, qOverload<>(&QTimer::start));
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, &m_debounce,
            qOverload<>(&QTimer::start));
}

void FileWatch::setPath(const QString& path) {
    if (!m_watcher.files().isEmpty())
        m_watcher.removePaths(m_watcher.files());
    if (!m_watcher.directories().isEmpty())
        m_watcher.removePaths(m_watcher.directories());
    m_path = path;
    rewatch();
}

void FileWatch::rewatch() {
    if (m_path.isEmpty())
        return;
    const QFileInfo info(m_path);
    // Watch the nearest existing ancestor so creating the file is noticed.
    QDir dir = info.dir();
    while (!dir.exists() && !dir.isRoot())
        dir.setPath(QFileInfo(dir.path()).path());
    if (!m_watcher.directories().contains(dir.path()))
        m_watcher.addPath(dir.path());
    if (info.exists() && !m_watcher.files().contains(m_path))
        m_watcher.addPath(m_path);
}

} // namespace island
