#pragma once

#include <QDBusContext>
#include <QHash>
#include <QObject>
#include <QVariantMap>

namespace island {

// Progress apps report for their taskbar icon (the Unity LauncherEntry
// signal: Chromium downloads, some installers and file managers) as live
// activities, while [live] app_progress is on. They end when the app hides
// its progress or leaves the bus.
class AppProgress : public QObject, protected QDBusContext {
    Q_OBJECT

public:
    static AppProgress* instance();

public slots:
    // com.canonical.Unity.LauncherEntry.Update
    void onUpdate(const QString& appUri, const QVariantMap& props);

private:
    explicit AppProgress(QObject* parent = nullptr);

    QHash<QString, double> m_progress; // live activity id → last progress
};

} // namespace island
