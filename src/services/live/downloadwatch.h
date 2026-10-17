#pragma once

#include <QDateTime>
#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QTimer>

namespace island {

// Browser downloads as live activities: a "name.part" (Firefox) or
// "name.crdownload" (Chromium) file in [live] downloads_directory is a
// download in progress, its size how far it got (browsers do not say the
// total there). When the partial file is gone and "name" exists, it is
// done. Only downloads that last a couple of seconds show up.
class DownloadWatch : public QObject {
    Q_OBJECT

public:
    static DownloadWatch* instance();

private:
    explicit DownloadWatch(QObject* parent = nullptr);
    void setup();
    void rescan();
    void tick();
    QString directory() const;

    struct Download {
        QString target;     // final file name
        QDateTime seen;     // when the partial file appeared
        qint64 size = 0;    // at the last tick
        bool shown = false; // has a live activity
    };
    QHash<QString, Download> m_active; // partial file name → download
    QString m_dir;
    QFileSystemWatcher m_watcher;
    QTimer m_tick;
};

} // namespace island
