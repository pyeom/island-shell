#include "progressparse.h"

#include <QDBusVariant>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace island {

namespace {
// Values may arrive wrapped in a QDBusVariant.
QVariant unwrap(const QVariant& v) {
    return v.canConvert<QDBusVariant>() ? v.value<QDBusVariant>().variant() : v;
}
} // namespace

LauncherEntryUpdate parseLauncherEntry(const QString& appUri, const QVariantMap& props) {
    LauncherEntryUpdate u;
    if (appUri.startsWith(u"application://"_s))
        u.desktopId = appUri.mid(14);
    if (const auto it = props.find(u"progress"_s); it != props.end()) {
        bool ok = false;
        const double p = unwrap(*it).toDouble(&ok);
        if (ok)
            u.progress = std::clamp(p, 0.0, 1.0);
    }
    if (const auto it = props.find(u"progress-visible"_s); it != props.end())
        u.progressVisible = unwrap(*it).toBool();
    return u;
}

QString partialDownloadTarget(const QString& fileName) {
    for (const auto& suffix : {u".part"_s, u".crdownload"_s}) {
        if (fileName.size() > suffix.size() && fileName.endsWith(suffix))
            return fileName.chopped(suffix.size());
    }
    return {};
}

QString formatBytes(qint64 bytes) {
    if (bytes < 1000 * 1000)
        return u"%1 KB"_s.arg(std::max<qint64>(bytes / 1000, bytes > 0 ? 1 : 0));
    if (bytes < 1000LL * 1000 * 1000)
        return u"%1 MB"_s.arg(static_cast<double>(bytes) / 1e6, 0, 'f', 1);
    return u"%1 GB"_s.arg(static_cast<double>(bytes) / 1e9, 0, 'f', 1);
}

} // namespace island
