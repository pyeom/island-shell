#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// Wallpapers in [wallpaper] directory, applied with [wallpaper] apply (%f is
// the file). Default apply: waypaper (or swww/awww), then pywal's `wal` when
// installed, so the island's pywal theme follows.
class Wallpapers : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QStringList files READ files NOTIFY changed)
    Q_PROPERTY(QString current READ current NOTIFY changed)

public:
    static Wallpapers* create(QQmlEngine*, QJSEngine*);

    QStringList files() const { return m_files; }
    QString current() const { return m_current; }

    Q_INVOKABLE void apply(const QString& file);
    Q_INVOKABLE void random();

signals:
    void changed();
    // The picker was requested (islandctl wallpaper).
    void pickRequested();

private:
    explicit Wallpapers(QObject* parent = nullptr);
    void scan();
    QString directory() const;
    QString applyCommand() const;

    QStringList m_files;
    QString m_current;
    QFileSystemWatcher m_watcher;
};

} // namespace island
