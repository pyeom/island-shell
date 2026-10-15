#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>

namespace island {

// Watches one file, including editors that save by replacing it (which drops
// a plain QFileSystemWatcher watch) and files that do not exist yet. Changes
// are debounced into a single `changed()`.
class FileWatch : public QObject {
    Q_OBJECT

public:
    explicit FileWatch(QObject* parent = nullptr);

    void setPath(const QString& path);
    QString path() const { return m_path; }

signals:
    void changed();

private:
    void rewatch();

    QString m_path;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
};

} // namespace island
