#pragma once

#include <QDateTime>
#include <QString>

#include <optional>

namespace island {

struct QuickEvent {
    QString summary;
    QDateTime start;
    QDateTime end;
    bool allDay = false;
};

// Parses "mañana 15:00 dentista", "vie 9 reunión 1h", "16/10 cine",
// "tomorrow 3pm dentist 30m". Spanish and English. Day: hoy/today,
// mañana/tomorrow, pasado mañana, weekday names (next occurrence; today if
// still ahead), d/m or d/m/y. Time: 15:00, 15h, 3pm, "a las 15" / "at 3".
// Duration: 1h, 30m, 1h30 (default 1 hour). No time: all-day.
// Returns nullopt (with `error`) when there is no title.
std::optional<QuickEvent> parseQuickEvent(const QString& text, const QDateTime& now,
                                          QString* error = nullptr);

// A VCALENDAR with one VEVENT; timed events in UTC, all-day ones as DATE.
QByteArray eventToIcs(const QuickEvent& event, const QString& uid, const QDateTime& stamp);

} // namespace island
