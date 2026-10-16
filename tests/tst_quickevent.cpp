#include "services/calendar/calendarparse.h"
#include "services/calendar/quickevent.h"

#include <QTest>

using namespace island;

namespace {
// Thursday 15 October 2026, 10:00 local.
const QDateTime kNow(QDate(2026, 10, 15), QTime(10, 0));
}

class TestQuickEvent : public QObject {
    Q_OBJECT

    static QuickEvent parse(const char* text) {
        QString error;
        const auto e = parseQuickEvent(QString::fromUtf8(text), kNow, &error);
        if (!e)
            qWarning() << text << error;
        return e.value_or(QuickEvent{});
    }

private slots:
    void tomorrowAtTime() {
        const auto e = parse("mañana 15:00 dentista");
        QCOMPARE(e.summary, QStringLiteral("dentista"));
        QCOMPARE(e.start, QDateTime(QDate(2026, 10, 16), QTime(15, 0)));
        QCOMPARE(e.end, QDateTime(QDate(2026, 10, 16), QTime(16, 0)));
        QVERIFY(!e.allDay);
    }

    void weekdayAndDuration() {
        const auto e = parse("vie a las 9 reunión de equipo 1h30");
        QCOMPARE(e.summary, QStringLiteral("reunión de equipo"));
        QCOMPARE(e.start, QDateTime(QDate(2026, 10, 16), QTime(9, 0)));
        QCOMPARE(e.end, e.start.addSecs(90 * 60));
    }

    void englishPm() {
        const auto e = parse("tomorrow 3pm call with Ana 30m");
        QCOMPARE(e.summary, QStringLiteral("call with Ana"));
        QCOMPARE(e.start.time(), QTime(15, 0));
        QCOMPARE(e.end, e.start.addSecs(30 * 60));
    }

    void dateWithoutTimeIsAllDay() {
        const auto e = parse("20/10 feriado");
        QVERIFY(e.allDay);
        QCOMPARE(e.start.date(), QDate(2026, 10, 20));
        QCOMPARE(e.end.date(), QDate(2026, 10, 21));
    }

    void pastDateMeansNextYear() {
        QCOMPARE(parse("1/3 cumpleaños").start.date(), QDate(2027, 3, 1));
    }

    void todayWeekdayAlreadyPassed() {
        // "jue 9" on Thursday at 10:00 is next Thursday.
        QCOMPARE(parse("jue 9h gimnasio").start.date(), QDate(2026, 10, 22));
        // Later today is still today.
        QCOMPARE(parse("jue 18:00 gimnasio").start.date(), QDate(2026, 10, 15));
    }

    void numbersStayInTitle() {
        const auto e = parse("mañana comprar 2 pizzas");
        QCOMPARE(e.summary, QStringLiteral("comprar 2 pizzas"));
        QVERIFY(e.allDay);
    }

    void missingTitle() {
        QString error;
        QVERIFY(!parseQuickEvent(QStringLiteral("mañana 15:00"), kNow, &error));
        QVERIFY(!error.isEmpty());
    }

    // What we write, libical reads back.
    void roundTrip() {
        const auto e = parse("mañana 15:00 dentista, control");
        const auto ics = eventToIcs(e, QStringLiteral("abc@island-shell"), kNow);
        const auto events = expandEvents(ics, kNow, kNow.addDays(7));
        QCOMPARE(events.size(), 1);
        QCOMPARE(events[0].summary, QStringLiteral("dentista, control"));
        QCOMPARE(events[0].start, e.start);
        QCOMPARE(events[0].end, e.end);

        const auto allDay = parse("20/10 feriado");
        const auto back = expandEvents(eventToIcs(allDay, QStringLiteral("x"), kNow), kNow, kNow.addDays(30));
        QCOMPARE(back.size(), 1);
        QVERIFY(back[0].allDay);
        QCOMPARE(back[0].start.date(), QDate(2026, 10, 20));
    }
};

QTEST_APPLESS_MAIN(TestQuickEvent)
#include "tst_quickevent.moc"
