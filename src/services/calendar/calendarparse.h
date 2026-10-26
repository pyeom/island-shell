#pragma once

#include <QDateTime>
#include <QList>
#include <QString>

namespace island {

struct CalendarEvent {
    QString uid;
    QString summary;
    QString location;
    QString description;
    QDateTime start; // local time
    QDateTime end;
    bool allDay = false;
    QString calendar; // collection display name
    QString color;    // collection color, may be empty
    QString path;     // the .ics file
};

// Every occurrence of every VEVENT in `ics` that overlaps [from, to), with
// recurrences (RRULE, RDATE, EXDATE) expanded and overridden instances
// (RECURRENCE-ID) replacing the ones they modify. Sorted by start.
QList<CalendarEvent> expandEvents(const QByteArray& ics, const QDateTime& from, const QDateTime& to);

} // namespace island
