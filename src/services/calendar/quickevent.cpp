#include "quickevent.h"

#include <QRegularExpression>
#include <QStringList>

namespace island {

namespace {

// Monday = 1 .. Sunday = 7.
int weekdayOf(const QString& word) {
    static const QList<QStringList> names = {
        {"lun", "lunes", "mon", "monday"},
        {"mar", "martes", "tue", "tues", "tuesday"},
        {"mie", "mié", "miercoles", "miércoles", "wed", "wednesday"},
        {"jue", "jueves", "thu", "thur", "thurs", "thursday"},
        {"vie", "viernes", "fri", "friday"},
        {"sab", "sáb", "sabado", "sábado", "sat", "saturday"},
        {"dom", "domingo", "sun", "sunday"},
    };
    for (int i = 0; i < names.size(); ++i)
        if (names[i].contains(word))
            return i + 1;
    return 0;
}

QString escapeText(QString s) {
    // RFC 5545 TEXT: backslash, semicolon, comma and newline are escaped.
    s.replace(u'\\', QStringLiteral("\\\\"));
    s.replace(u';', QStringLiteral("\\;"));
    s.replace(u',', QStringLiteral("\\,"));
    s.replace(u'\n', QStringLiteral("\\n"));
    return s;
}

} // namespace

std::optional<QuickEvent> parseQuickEvent(const QString& text, const QDateTime& now, QString* error) {
    QStringList words = text.simplified().split(u' ', Qt::SkipEmptyParts);
    std::optional<QDate> day;
    std::optional<QTime> time;
    int minutes = 60;
    bool explicitDuration = false;
    QStringList title;

    static const QRegularExpression clock(QStringLiteral("^(\\d{1,2})(?::(\\d{2}))?(am|pm|h)?$"));
    // 1h, 90m, 1h30, 1h30m (a bare number never counts: it needs h or m).
    static const QRegularExpression duration(QStringLiteral("^(?:(\\d+)h)?(?:(\\d+)(?:m|min)?)?$"));
    static const QRegularExpression dmY(QStringLiteral("^(\\d{1,2})[/-](\\d{1,2})(?:[/-](\\d{2,4}))?$"));

    bool expectTime = false; // after "a las" / "at"
    for (qsizetype i = 0; i < words.size(); ++i) {
        const QString w = words[i].toLower();
        const QString next = i + 1 < words.size() ? words[i + 1].toLower() : QString();

        if (!day && (w == u"hoy" || w == u"today")) {
            day = now.date();
            continue;
        }
        if (!day && w == u"pasado" && next == u"mañana") {
            day = now.date().addDays(2);
            ++i;
            continue;
        }
        if (!day && (w == u"mañana" || w == u"manana" || w == u"tomorrow")) {
            day = now.date().addDays(1);
            continue;
        }
        if (!day && weekdayOf(w)) {
            int delta = (weekdayOf(w) - now.date().dayOfWeek() + 7) % 7;
            day = now.date().addDays(delta); // today counts; checked against the time below
            continue;
        }
        if (!day) {
            if (const auto m = dmY.match(w); m.hasMatch()) {
                int year = m.captured(3).isEmpty() ? now.date().year() : m.captured(3).toInt();
                if (year < 100)
                    year += 2000;
                const QDate d(year, m.captured(2).toInt(), m.captured(1).toInt());
                if (d.isValid()) {
                    // Without a year, a date already past means next year.
                    day = (m.captured(3).isEmpty() && d < now.date()) ? d.addYears(1) : d;
                    continue;
                }
            }
        }
        if ((w == u"a" && (next == u"las" || next == u"la")) || w == u"at") {
            expectTime = true;
            if (w == u"a")
                ++i;
            continue;
        }
        if (!time) {
            if (const auto m = clock.match(w); m.hasMatch()) {
                // A bare number is a time only after "a las"/"at" or with a
                // marker (15h, 3pm, 15:00); otherwise it is part of the title.
                const bool marked = !m.captured(2).isEmpty() || !m.captured(3).isEmpty();
                if (marked || expectTime) {
                    int h = m.captured(1).toInt();
                    const int min = m.captured(2).isEmpty() ? 0 : m.captured(2).toInt();
                    if (m.captured(3) == u"pm" && h < 12)
                        h += 12;
                    if (m.captured(3) == u"am" && h == 12)
                        h = 0;
                    const QTime t(h, min);
                    if (t.isValid()) {
                        time = t;
                        expectTime = false;
                        continue;
                    }
                }
            }
        }
        if (!explicitDuration && (w.endsWith(u'h') || w.endsWith(u'm') || w.endsWith(QLatin1String("min")) ||
                                  w.contains(u'h'))) {
            if (const auto m = duration.match(w); m.hasMatch() && !w.isEmpty() &&
                                                  (m.captured(1).size() || m.captured(2).size())) {
                const int total = m.captured(1).toInt() * 60 + m.captured(2).toInt();
                if (total > 0) {
                    minutes = total;
                    explicitDuration = true;
                    continue;
                }
            }
        }
        expectTime = false;
        title.append(words[i]);
    }

    QuickEvent e;
    e.summary = title.join(u' ');
    if (e.summary.isEmpty()) {
        if (error)
            *error = QStringLiteral("missing title");
        return std::nullopt;
    }
    if (!day)
        day = now.date();
    if (time) {
        e.start = QDateTime(*day, *time);
        // "vie 9" said on a Friday at 10:00 means next Friday.
        if (e.start < now && day == now.date() && words.size() > 0 &&
            weekdayOf(words.first().toLower()))
            e.start = e.start.addDays(7);
        e.end = e.start.addSecs(minutes * 60LL);
    } else {
        e.allDay = true;
        e.start = day->startOfDay();
        e.end = day->addDays(1).startOfDay();
    }
    return e;
}

QByteArray eventToIcs(const QuickEvent& event, const QString& uid, const QDateTime& stamp) {
    const auto utc = [](const QDateTime& dt) {
        return dt.toUTC().toString(QStringLiteral("yyyyMMdd'T'HHmmss'Z'"));
    };
    QStringList lines = {
        QStringLiteral("BEGIN:VCALENDAR"),
        QStringLiteral("VERSION:2.0"),
        QStringLiteral("PRODID:-//island-shell//EN"),
        QStringLiteral("BEGIN:VEVENT"),
        QStringLiteral("UID:") + uid,
        QStringLiteral("DTSTAMP:") + utc(stamp),
        QStringLiteral("SUMMARY:") + escapeText(event.summary),
    };
    if (event.allDay) {
        lines << QStringLiteral("DTSTART;VALUE=DATE:") + event.start.date().toString(QStringLiteral("yyyyMMdd"))
              << QStringLiteral("DTEND;VALUE=DATE:") + event.end.date().toString(QStringLiteral("yyyyMMdd"));
    } else {
        lines << QStringLiteral("DTSTART:") + utc(event.start) << QStringLiteral("DTEND:") + utc(event.end);
    }
    lines << QStringLiteral("END:VEVENT") << QStringLiteral("END:VCALENDAR");
    return (lines.join(QStringLiteral("\r\n")) + QStringLiteral("\r\n")).toUtf8();
}

} // namespace island
