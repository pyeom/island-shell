#include "services/system/sysparse.h"

#include <QTest>

using namespace island::sys;

class TestSysParse : public QObject {
    Q_OBJECT

private slots:
    void procStat() {
        const auto t = parseProcStat("cpu  100 0 50 800 50 0 0 0 0 0\ncpu0 1 2 3 4\n");
        QVERIFY(t);
        QCOMPARE(t->total, quint64(1000));
        QCOMPARE(t->idle, quint64(850));
        QVERIFY(!parseProcStat("intr 1 2 3"));
    }

    void usage() {
        const CpuTimes a{.idle = 800, .total = 1000};
        const CpuTimes b{.idle = 850, .total = 1200}; // 150 of 200 busy
        QCOMPARE(cpuUsage(a, b), 0.75);
        QCOMPARE(cpuUsage(b, a), 0.0); // counters went backwards
    }

    void meminfo() {
        const auto m = parseMeminfo("MemTotal:       32806624 kB\nMemFree: 1 kB\n"
                                    "MemAvailable:   24604968 kB\n");
        QVERIFY(m);
        QCOMPARE(m->totalKb, quint64(32806624));
        QVERIFY(std::abs(m->usedFraction() - 0.25) < 0.001);
        QVERIFY(!parseMeminfo("garbage"));
    }
};

QTEST_APPLESS_MAIN(TestSysParse)
#include "tst_sysparse.moc"
