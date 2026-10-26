#include "timerparse.h"

#include <QTest>

using namespace island;

class TestTimerParse : public QObject {
    Q_OBJECT

private slots:
    void durations_data() {
        QTest::addColumn<QString>("text");
        QTest::addColumn<int>("seconds");
        QTest::addColumn<QString>("label");
        QTest::newRow("min") << "15 min" << 900 << "";
        QTest::newRow("m") << "15m" << 900 << "";
        QTest::newRow("minutes") << "2 minutes" << 120 << "";
        QTest::newRow("minutos") << "10 minutos" << 600 << "";
        QTest::newRow("quote") << "15'" << 900 << "";
        QTest::newRow("two quotes") << "15''" << 15 << "";
        QTest::newRow("double quote") << "90\"" << 90 << "";
        QTest::newRow("hour") << "1 hour" << 3600 << "";
        QTest::newRow("h") << "1 h" << 3600 << "";
        QTest::newRow("horas") << "2 horas" << 7200 << "";
        QTest::newRow("h and bare minutes") << "1h30" << 5400 << "";
        QTest::newRow("h and m") << "1h 30m" << 5400 << "";
        QTest::newRow("min and quote seconds") << "1' 30''" << 90 << "";
        QTest::newRow("decimal") << "1.5 h" << 5400 << "";
        QTest::newRow("decimal comma") << "1,5 h" << 5400 << "";
        QTest::newRow("seconds") << "45 s" << 45 << "";
        QTest::newRow("label") << "15 min pasta" << 900 << "pasta";
        QTest::newRow("prefix") << "timer 10 min" << 600 << "";
        QTest::newRow("prefix bare") << "timer 5" << 300 << "";
        QTest::newRow("prefix bare label") << "timer 5 té" << 300 << "té";
        QTest::newRow("temporizador") << "Temporizador 3 min huevos" << 180 << "huevos";
        QTest::newRow("case") << "15 MIN" << 900 << "";
    }
    void durations() {
        QFETCH(QString, text);
        QFETCH(int, seconds);
        QFETCH(QString, label);
        const auto t = parseTimer(text);
        QVERIFY2(t, qPrintable(text));
        QCOMPARE(t->seconds, seconds);
        QCOMPARE(t->label, label);
    }

    void notTimers_data() {
        QTest::addColumn<QString>("text");
        for (const char* s : {"", "15", "firefox", "15 minecraft", "3 monitors", "timer", "0 min",
                              "15mx", "2*3", "1000 hours", "min 15", "timerx 5"})
            QTest::newRow(s) << QString::fromUtf8(s);
    }
    void notTimers() {
        QFETCH(QString, text);
        QVERIFY2(!parseTimer(text), qPrintable(text));
    }

    void format() {
        QCOMPARE(formatDuration(900), QStringLiteral("15 min"));
        QCOMPARE(formatDuration(5400), QStringLiteral("1 h 30 min"));
        QCOMPARE(formatDuration(45), QStringLiteral("45 s"));
        QCOMPARE(formatDuration(125), QStringLiteral("2 min 5 s"));
    }
};

QTEST_GUILESS_MAIN(TestTimerParse)
#include "tst_timerparse.moc"
