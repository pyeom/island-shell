#include "volumestep.h"

#include <QTest>

using namespace island;

class TestVolumeStep : public QObject {
    Q_OBJECT

private slots:
    void steps() {
        QCOMPARE(*volumeTarget(u"up", 0.43), 0.48);
        QCOMPARE(*volumeTarget(u"down", 0.43), 0.38);
        QCOMPARE(*volumeTarget(u"down", 0.02), 0.0);
        QCOMPARE(*volumeTarget(u"+10", 0.5), 0.6);
        QCOMPARE(*volumeTarget(u"-10", 0.5), 0.4);
    }

    void stepsStopAtFullUnlessAbove() {
        QCOMPARE(*volumeTarget(u"up", 0.98), 1.0);
        QCOMPARE(*volumeTarget(u"up", 1.0), 1.0);
        QCOMPARE(*volumeTarget(u"up", 1.2), 1.2); // set above 100% elsewhere: not lowered
        QCOMPARE(*volumeTarget(u"down", 1.2), 1.15);
    }

    void set() {
        QCOMPARE(*volumeTarget(u"40", 0.9), 0.4);
        QCOMPARE(*volumeTarget(u"130", 0.2), 1.3);
        QCOMPARE(*volumeTarget(u"400", 0.2), 1.5);
    }

    void invalid() {
        QVERIFY(!volumeTarget(u"loud", 0.5));
        QVERIFY(!volumeTarget(u"", 0.5));
        QVERIFY(!volumeTarget(u"nan", 0.5));
        QVERIFY(!volumeTarget(u"inf", 0.5));
    }
};

QTEST_GUILESS_MAIN(TestVolumeStep)
#include "tst_volumestep.moc"
