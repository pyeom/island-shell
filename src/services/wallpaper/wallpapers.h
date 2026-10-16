#pragma once

#include "wallpaperbackend.h"

#include <QFileSystemWatcher>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;
class QProcess;

namespace island {

// Wallpapers in [wallpaper] directory, set through [wallpaper] backend:
// awww, swww, hyprpaper, swaybg, waypaper or a custom command. "auto" picks
// waypaper when it is set up (it restores the wallpaper at login), else the
// daemon that is running, else the first one installed. pywal's `wal` runs
// afterwards when installed, so the island's pywal theme follows.
//
// `current` is what the backend shows (on the first output), so changes made
// outside the island count too. When the backend cannot be asked, it is the
// newest of pywal's cache, waypaper's config and the island's last choice.
class Wallpapers : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QStringList files READ files NOTIFY changed)
    Q_PROPERTY(QString current READ current NOTIFY changed)
    // The backend in use, after resolving "auto": "awww", ..., "none".
    Q_PROPERTY(QString backend READ backend NOTIFY changed)

public:
    static Wallpapers* create(QQmlEngine*, QJSEngine*);

    QStringList files() const { return m_files; }
    QString current() const { return m_current; }
    QString backend() const { return wallpaper::backendName(m_backend); }

    Q_INVOKABLE void apply(const QString& file);
    Q_INVOKABLE void random();
    // Asks the backend again, for changes made by other tools (the picker
    // calls it when it opens).
    Q_INVOKABLE void refresh() { queryBackend(); }

signals:
    void changed();

private:
    explicit Wallpapers(QObject* parent = nullptr);
    void scan();
    wallpaper::Backend resolveBackend() const;
    // Asks the backend what it shows; resolveCurrent() runs with the answer.
    void queryBackend();
    void queryDone(QProcess* process);
    void resolveCurrent();
    QString directory() const;
    static QString statePath();
    static QString walCachePath();
    static QString waypaperIniPath();

    QStringList m_files;
    QString m_current;
    wallpaper::Backend m_backend = wallpaper::Backend::None;
    // Images the backend reported, one per output; empty when unknown.
    QStringList m_shown;
    QPointer<QProcess> m_query;
    bool m_queryAgain = false;
    QFileSystemWatcher m_watcher;
};

} // namespace island
