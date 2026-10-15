#include "calendarparse.h"

#include <QSet>
#include <QTimeZone>

#include <libical/ical.h>

#include <algorithm>
#include <memory>

namespace island {

namespace {

struct ComponentDeleter {
    void operator()(icalcomponent* c) const { icalcomponent_free(c); }
};
using ComponentPtr = std::unique_ptr<icalcomponent, ComponentDeleter>;

QString text(icalcomponent* event, icalproperty_kind kind) {
    icalproperty* p = icalcomponent_get_first_property(event, kind);
    if (!p)
        return {};
    const char* v = icalproperty_get_value_as_string(p);
    return v ? QString::fromUtf8(v) : QString();
}

QString stringProp(icalcomponent* event, icalproperty_kind kind) {
    icalproperty* p = icalcomponent_get_first_property(event, kind);
    if (!p)
        return {};
    // Unescaped value (\n, \, -> real characters).
    icalvalue* v = icalproperty_get_value(p);
    const char* s = v ? icalvalue_get_text(v) : nullptr;
    return s ? QString::fromUtf8(s) : text(event, kind);
}

icaltimetype toIcal(const QDateTime& dt) {
    return icaltime_from_timet_with_zone(dt.toSecsSinceEpoch(), 0, icaltimezone_get_utc_timezone());
}

// Key identifying one occurrence: uid + start in UTC seconds.
QString occurrenceKey(const QString& uid, qint64 utcStart) {
    return uid + u'@' + QString::number(utcStart);
}

struct Collector {
    QString uid;
    bool allDay = false;
    qint64 durationSecs = 0;
    const QSet<QString>* overridden = nullptr;
    QList<qint64> starts; // UTC seconds
};

void collect(icalcomponent* /*comp*/, const icaltime_span* span, void* data) {
    auto* c = static_cast<Collector*>(data);
    if (c->overridden->contains(occurrenceKey(c->uid, span->start)))
        return;
    c->starts.append(span->start);
}

} // namespace

QList<CalendarEvent> expandEvents(const QByteArray& ics, const QDateTime& from, const QDateTime& to) {
    QList<CalendarEvent> out;
    ComponentPtr root(icalparser_parse_string(ics.constData()));
    if (!root)
        return out;

    // A file may be a VCALENDAR or a bare component.
    icalcomponent* cal = root.get();
    auto events = [cal](auto&& fn) {
        if (icalcomponent_isa(cal) == ICAL_VEVENT_COMPONENT) {
            fn(cal);
            return;
        }
        for (icalcomponent* e = icalcomponent_get_first_component(cal, ICAL_VEVENT_COMPONENT); e;
             e = icalcomponent_get_next_component(cal, ICAL_VEVENT_COMPONENT))
            fn(e);
    };

    // Overridden instances first, so the master skips them.
    QSet<QString> overridden;
    events([&](icalcomponent* e) {
        const icaltimetype rid = icalcomponent_get_recurrenceid(e);
        if (icaltime_is_null_time(rid))
            return;
        const char* uid = icalcomponent_get_uid(e);
        overridden.insert(occurrenceKey(QString::fromUtf8(uid ? uid : ""),
                                        icaltime_as_timet_with_zone(rid, rid.zone)));
    });

    const icaltimetype windowStart = toIcal(from);
    const icaltimetype windowEnd = toIcal(to);

    events([&](icalcomponent* e) {
        const icaltimetype dtstart = icalcomponent_get_dtstart(e);
        if (icaltime_is_null_time(dtstart))
            return;
        const char* uidRaw = icalcomponent_get_uid(e);
        const QString uid = QString::fromUtf8(uidRaw ? uidRaw : "");
        const bool isOverride = !icaltime_is_null_time(icalcomponent_get_recurrenceid(e));

        icaltimetype dtend = icalcomponent_get_dtend(e);
        const bool allDay = dtstart.is_date;
        qint64 duration = 0;
        if (!icaltime_is_null_time(dtend))
            duration = icaltime_as_timet_with_zone(dtend, dtend.zone) -
                       icaltime_as_timet_with_zone(dtstart, dtstart.zone);
        else if (allDay)
            duration = 24 * 3600;

        Collector c{uid, allDay, duration, isOverride ? nullptr : &overridden, {}};
        if (isOverride) {
            // A modified instance stands alone.
            c.starts.append(icaltime_as_timet_with_zone(dtstart, dtstart.zone));
        } else {
            icalcomponent_foreach_recurrence(e, windowStart, windowEnd, collect, &c);
        }

        const QString summary = stringProp(e, ICAL_SUMMARY_PROPERTY);
        const QString location = stringProp(e, ICAL_LOCATION_PROPERTY);
        const QString description = stringProp(e, ICAL_DESCRIPTION_PROPERTY);
        for (const qint64 utc : std::as_const(c.starts)) {
            CalendarEvent ev;
            ev.uid = uid;
            ev.summary = summary;
            ev.location = location;
            ev.description = description;
            ev.allDay = allDay;
            if (allDay) {
                // Dates are floating: the same calendar day everywhere.
                const QDate day = QDateTime::fromSecsSinceEpoch(utc, QTimeZone::UTC).date();
                ev.start = day.startOfDay();
                ev.end = day.addDays(std::max<qint64>(1, duration / 86400)).startOfDay();
            } else {
                ev.start = QDateTime::fromSecsSinceEpoch(utc).toLocalTime();
                ev.end = ev.start.addSecs(std::max<qint64>(duration, 0));
            }
            if (ev.end > from && ev.start < to)
                out.append(ev);
        }
    });

    std::ranges::sort(out, {}, &CalendarEvent::start);
    return out;
}

} // namespace island
