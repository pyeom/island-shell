#pragma once

#include <QByteArrayView>

#include <optional>

namespace island::sys {

struct CpuTimes {
    quint64 idle = 0;  // idle + iowait
    quint64 total = 0;
};

// First line of /proc/stat ("cpu  user nice system idle iowait ...").
std::optional<CpuTimes> parseProcStat(QByteArrayView text);

// Busy fraction 0..1 between two samples.
double cpuUsage(const CpuTimes& before, const CpuTimes& after);

struct Memory {
    quint64 totalKb = 0;
    quint64 availableKb = 0;
    [[nodiscard]] double usedFraction() const {
        return totalKb ? 1.0 - static_cast<double>(availableKb) / static_cast<double>(totalKb) : 0;
    }
};

std::optional<Memory> parseMeminfo(QByteArrayView text);

} // namespace island::sys
