#pragma once

#include <QString>
#include <QVariantMap>

#include <optional>

namespace island {

// com.canonical.Unity.LauncherEntry.Update("application://firefox.desktop",
// { progress: 0.4, "progress-visible": true, count: 3, ... }), the signal
// apps send for progress on their taskbar icon.
struct LauncherEntryUpdate {
    QString desktopId; // "firefox.desktop"; empty for other URIs
    std::optional<double> progress;
    std::optional<bool> progressVisible;
    std::optional<qint64> count; // unread messages...
    std::optional<bool> countVisible;
};
LauncherEntryUpdate parseLauncherEntry(const QString& appUri, const QVariantMap& props);

// The file a browser is downloading into `fileName`: "linux.iso.part"
// (Firefox) or "linux.iso.crdownload" (Chromium) → "linux.iso"; empty for
// anything else.
QString partialDownloadTarget(const QString& fileName);

// "850 KB", "12.4 MB", "1.2 GB".
QString formatBytes(qint64 bytes);

} // namespace island
