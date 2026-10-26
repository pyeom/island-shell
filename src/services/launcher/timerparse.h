#pragma once

#include <QString>
#include <QStringView>

#include <optional>

namespace island {

struct TimerRequest {
    int seconds = 0;
    QString label; // what follows the duration, may be empty
};

// A timer typed in the launcher: a duration, optionally after "timer" or
// "temporizador", optionally followed by a label.
//   15 min · 15m · 15' · 90 s · 90'' · 90" · 1 hour · 1 h · 1h30 · 1h 30m
//   1.5 h · 2 horas · 10 minutos · timer 5 · timer 15 min pasta
// A bare number only counts after "timer" (minutes). Nothing for ordinary
// text, zero, or more than a week.
std::optional<TimerRequest> parseTimer(QStringView text);

// "1 h 30 min", "15 min", "45 s", "2 min 5 s".
QString formatDuration(int seconds);

} // namespace island
