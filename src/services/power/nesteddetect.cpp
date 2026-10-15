#include "nesteddetect.h"

#include <QFile>

namespace island {

bool environIsNested(QByteArrayView environ) {
    qsizetype start = 0;
    while (start < environ.size()) {
        qsizetype end = environ.indexOf('\0', start);
        if (end < 0)
            end = environ.size();
        const auto entry = environ.sliced(start, end - start);
        if (entry.startsWith("WAYLAND_DISPLAY=") && entry.size() > 16)
            return true;
        start = end + 1;
    }
    return false;
}

bool runningUnderNestedHyprland() {
    const auto signature = qEnvironmentVariable("HYPRLAND_INSTANCE_SIGNATURE");
    auto runtime = qEnvironmentVariable("XDG_RUNTIME_DIR");
    if (signature.isEmpty() || runtime.isEmpty())
        return false;
    QFile lock(runtime + QLatin1String("/hypr/") + signature + QLatin1String("/hyprland.lock"));
    if (!lock.open(QIODevice::ReadOnly))
        return false;
    bool ok = false;
    const auto pid = lock.readLine().trimmed().toLongLong(&ok);
    if (!ok || pid <= 0)
        return false;
    QFile environ(QStringLiteral("/proc/%1/environ").arg(pid));
    return environ.open(QIODevice::ReadOnly) && environIsNested(environ.readAll());
}

} // namespace island
