#include "sysparse.h"

#include <QByteArray>
#include <QList>

#include <algorithm>

namespace island::sys {

std::optional<CpuTimes> parseProcStat(QByteArrayView text) {
    const auto nl = text.indexOf('\n');
    const auto line = nl >= 0 ? text.first(nl) : text;
    if (!line.startsWith("cpu "))
        return std::nullopt;
    const auto fields = line.toByteArray().simplified().split(' ');
    if (fields.size() < 5)
        return std::nullopt;
    CpuTimes t;
    for (qsizetype i = 1; i < fields.size() && i <= 8; ++i) { // up to steal
        bool ok = false;
        const quint64 v = fields[i].toULongLong(&ok);
        if (!ok)
            return std::nullopt;
        t.total += v;
        if (i == 4 || i == 5) // idle, iowait
            t.idle += v;
    }
    return t;
}

double cpuUsage(const CpuTimes& before, const CpuTimes& after) {
    if (after.total <= before.total)
        return 0;
    const double total = static_cast<double>(after.total - before.total);
    const double idle = static_cast<double>(after.idle - before.idle);
    return std::clamp(1.0 - idle / total, 0.0, 1.0);
}

std::optional<Memory> parseMeminfo(QByteArrayView text) {
    Memory m;
    for (const auto& line : text.toByteArray().split('\n')) {
        const auto value = [&] { return line.mid(line.indexOf(':') + 1).trimmed().split(' ').value(0).toULongLong(); };
        if (line.startsWith("MemTotal:"))
            m.totalKb = value();
        else if (line.startsWith("MemAvailable:"))
            m.availableKb = value();
    }
    if (m.totalKb == 0)
        return std::nullopt;
    return m;
}

} // namespace island::sys
