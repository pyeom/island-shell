#include "anim/springmodel.h"

#include <QTest>

using island::SpringModel;

class TestSpringModel : public QObject {
    Q_OBJECT

private:
    // Runs at a fixed frame rate until settled; returns peak value and frames.
    static std::pair<double, int> run(SpringModel& s, double fps, int maxFrames = 2000) {
        double peak = s.value;
        int frames = 0;
        while (!s.settled() && frames < maxFrames) {
            s.step(1.0 / fps);
            peak = std::max(peak, s.value);
            ++frames;
        }
        return {peak, frames};
    }

private slots:
    void settlesOnTarget() {
        SpringModel s;
        s.target = 100.0;
        const auto [peak, frames] = run(s, 144.0);
        Q_UNUSED(peak);
        QVERIFY(s.settled());
        QVERIFY(frames < 2000);
        QVERIFY(std::abs(s.value - 100.0) < 0.01);
    }

    void underdampedOvershoots() {
        SpringModel s;
        s.setResponse(0.4, 0.5);
        s.target = 100.0;
        const auto [peak, frames] = run(s, 144.0);
        Q_UNUSED(frames);
        QVERIFY(peak > 100.5);
    }

    void criticallyDampedDoesNotOvershoot() {
        SpringModel s;
        s.setResponse(0.4, 1.0);
        s.target = 100.0;
        const auto [peak, frames] = run(s, 144.0);
        Q_UNUSED(frames);
        QVERIFY(peak <= 100.0 + 1e-6);
    }

    // The same spring must look the same on a 60 Hz and a 144 Hz monitor.
    void independentOfRefreshRate() {
        SpringModel a;
        SpringModel b;
        a.target = b.target = 100.0;
        const double duration = 0.25;
        for (int i = 0; i < static_cast<int>(duration * 60); ++i)
            a.step(1.0 / 60.0);
        for (int i = 0; i < static_cast<int>(duration * 144); ++i)
            b.step(1.0 / 144.0);
        QVERIFY(std::abs(a.value - b.value) < 0.5);
    }

    void stallIsClamped() {
        SpringModel s;
        s.target = 100.0;
        s.step(5.0);
        QVERIFY(s.value < 100.0);
        QVERIFY(std::isfinite(s.value));
    }

    void snapStops() {
        SpringModel s;
        s.target = 42.0;
        s.step(0.01);
        s.snap();
        QCOMPARE(s.value, 42.0);
        QCOMPARE(s.velocity, 0.0);
        QVERIFY(s.settled());
    }
};

QTEST_APPLESS_MAIN(TestSpringModel)
#include "tst_springmodel.moc"
