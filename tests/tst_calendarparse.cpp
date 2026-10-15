#include "services/calendar/calendarparse.h"

#include <QTest>
#include <QTimeZone>

using namespace island;

namespace {

QByteArray calendar(const QByteArray& body) {
    return "BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-//test//EN\r\n" + body + "END:VCALENDAR\r\n";
}

QDateTime utc(int y, int m, int d, int h = 0, int min = 0) {
    return QDateTime(QDate(y, m, d), QTime(h, min), QTimeZone::UTC);
}

} // namespace

class TestCalendarParse : public QObject {
    Q_OBJECT

private slots:
    void singleEvent() {
        const auto ics = calendar("BEGIN:VEVENT\r\nUID:a\r\nSUMMARY:Dentista\r\n"
                                  "LOCATION:Calle 1\\, Santiago\r\n"
                                  "DTSTART:20261020T130000Z\r\nDTEND:20261020T140000Z\r\n"
                                  "END:VEVENT\r\n");
        const auto events = expandEvents(ics, utc(2026, 10, 1), utc(2026, 11, 1));
        QCOMPARE(events.size(), 1);
        QCOMPARE(events[0].summary, QStringLiteral("Dentista"));
        QCOMPARE(events[0].location, QStringLiteral("Calle 1, Santiago"));
        QCOMPARE(events[0].start.toUTC(), utc(2026, 10, 20, 13));
        QCOMPARE(events[0].end.toUTC(), utc(2026, 10, 20, 14));
        QVERIFY(!events[0].allDay);
    }

    void outsideWindowIsDropped() {
        const auto ics = calendar("BEGIN:VEVENT\r\nUID:a\r\nSUMMARY:x\r\n"
                                  "DTSTART:20250101T100000Z\r\nDTEND:20250101T110000Z\r\n"
                                  "END:VEVENT\r\n");
        QVERIFY(expandEvents(ics, utc(2026, 10, 1), utc(2026, 11, 1)).isEmpty());
    }

    void weeklyWithExdate() {
        const auto ics = calendar("BEGIN:VEVENT\r\nUID:w\r\nSUMMARY:Standup\r\n"
                                  "DTSTART:20261005T120000Z\r\nDTEND:20261005T121500Z\r\n"
                                  "RRULE:FREQ=WEEKLY;COUNT=4\r\n"
                                  "EXDATE:20261012T120000Z\r\n"
                                  "END:VEVENT\r\n");
        const auto events = expandEvents(ics, utc(2026, 10, 1), utc(2026, 12, 1));
        QCOMPARE(events.size(), 3); // 5th, 19th, 26th; 12th excluded
        QCOMPARE(events[0].start.toUTC(), utc(2026, 10, 5, 12));
        QCOMPARE(events[1].start.toUTC(), utc(2026, 10, 19, 12));
        QCOMPARE(events[2].end.toUTC(), utc(2026, 10, 26, 12, 15));
    }

    void overriddenInstanceReplacesOccurrence() {
        const auto ics = calendar("BEGIN:VEVENT\r\nUID:r\r\nSUMMARY:Clase\r\n"
                                  "DTSTART:20261005T150000Z\r\nDTEND:20261005T160000Z\r\n"
                                  "RRULE:FREQ=DAILY;COUNT=3\r\nEND:VEVENT\r\n"
                                  "BEGIN:VEVENT\r\nUID:r\r\nSUMMARY:Clase (movida)\r\n"
                                  "RECURRENCE-ID:20261006T150000Z\r\n"
                                  "DTSTART:20261006T180000Z\r\nDTEND:20261006T190000Z\r\n"
                                  "END:VEVENT\r\n");
        const auto events = expandEvents(ics, utc(2026, 10, 1), utc(2026, 10, 31));
        QCOMPARE(events.size(), 3);
        QCOMPARE(events[1].summary, QStringLiteral("Clase (movida)"));
        QCOMPARE(events[1].start.toUTC(), utc(2026, 10, 6, 18));
    }

    void allDayKeepsItsDate() {
        const auto ics = calendar("BEGIN:VEVENT\r\nUID:d\r\nSUMMARY:Feriado\r\n"
                                  "DTSTART;VALUE=DATE:20261012\r\nDTEND;VALUE=DATE:20261013\r\n"
                                  "END:VEVENT\r\n");
        const auto events = expandEvents(ics, utc(2026, 10, 1), utc(2026, 11, 1));
        QCOMPARE(events.size(), 1);
        QVERIFY(events[0].allDay);
        QCOMPARE(events[0].start.date(), QDate(2026, 10, 12));
        QCOMPARE(events[0].end.date(), QDate(2026, 10, 13));
    }

    void timezoneFromVtimezone() {
        // 10:00 in Santiago (UTC-3 in October 2026) is 13:00 UTC.
        const auto ics = calendar("BEGIN:VTIMEZONE\r\nTZID:America/Santiago\r\n"
                                  "BEGIN:DAYLIGHT\r\nTZOFFSETFROM:-0400\r\nTZOFFSETTO:-0300\r\n"
                                  "DTSTART:19700906T000000\r\nRRULE:FREQ=YEARLY;BYMONTH=9;BYDAY=1SU\r\n"
                                  "END:DAYLIGHT\r\n"
                                  "BEGIN:STANDARD\r\nTZOFFSETFROM:-0300\r\nTZOFFSETTO:-0400\r\n"
                                  "DTSTART:19700405T000000\r\nRRULE:FREQ=YEARLY;BYMONTH=4;BYDAY=1SU\r\n"
                                  "END:STANDARD\r\nEND:VTIMEZONE\r\n"
                                  "BEGIN:VEVENT\r\nUID:tz\r\nSUMMARY:Reunión\r\n"
                                  "DTSTART;TZID=America/Santiago:20261020T100000\r\n"
                                  "DTEND;TZID=America/Santiago:20261020T110000\r\n"
                                  "END:VEVENT\r\n");
        const auto events = expandEvents(ics, utc(2026, 10, 1), utc(2026, 11, 1));
        QCOMPARE(events.size(), 1);
        QCOMPARE(events[0].start.toUTC(), utc(2026, 10, 20, 13));
    }

    // Many exports name a TZID without including its VTIMEZONE.
    void timezoneWithoutVtimezone() {
        const auto ics = calendar("BEGIN:VEVENT\r\nUID:tz2\r\nSUMMARY:x\r\n"
                                  "DTSTART;TZID=Europe/Madrid:20261020T100000\r\n"
                                  "DTEND;TZID=Europe/Madrid:20261020T110000\r\n"
                                  "END:VEVENT\r\n");
        const auto events = expandEvents(ics, utc(2026, 10, 1), utc(2026, 11, 1));
        QCOMPARE(events.size(), 1);
        // CEST, UTC+2.
        QCOMPARE(events[0].start.toUTC(), utc(2026, 10, 20, 8));
    }

    void garbage() {
        QVERIFY(expandEvents("not a calendar", utc(2026, 1, 1), utc(2027, 1, 1)).isEmpty());
        QVERIFY(expandEvents("", utc(2026, 1, 1), utc(2027, 1, 1)).isEmpty());
    }
};

QTEST_APPLESS_MAIN(TestCalendarParse)
#include "tst_calendarparse.moc"
