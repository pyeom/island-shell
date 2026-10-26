// Integration test: LauncherSearch against the module's real services
// (Apps scans the system's .desktop files; Hyprland is disabled).
#include "calendar.h"
#include "config.h"
#include "launchersearch.h"
#include "timers.h"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QTest>

using island::LauncherSearch;

class TestLauncherSearch : public QObject {
    Q_OBJECT

    LauncherSearch* search = nullptr;
    QTemporaryDir tmp;

    QVariantMap first(const QString& query) {
        const auto results = search->query(query);
        return results.isEmpty() ? QVariantMap() : results.first().toMap();
    }

private slots:
    void initTestCase() {
        qunsetenv("HYPRLAND_INSTANCE_SIGNATURE"); // never talk to the real compositor
        // A throwaway vdir with one collection, so "+" events have somewhere to go.
        QDir(tmp.path()).mkpath(QStringLiteral("cal/personal"));
        QFile meta(tmp.filePath(QStringLiteral("cal/personal/displayname")));
        QVERIFY(meta.open(QIODevice::WriteOnly));
        meta.write("Personal");
        meta.close();
        QFile config(tmp.filePath(QStringLiteral("config.toml")));
        QVERIFY(config.open(QIODevice::WriteOnly));
        config.write("[calendar]\npaths = [\"" + tmp.filePath(QStringLiteral("cal")).toUtf8() + "\"]\n");
        config.close();
        qputenv("ISLAND_SHELL_CONFIG", tmp.filePath(QStringLiteral("config.toml")).toUtf8());
        search = LauncherSearch::create(nullptr, nullptr);
        island::Calendar::create(nullptr, nullptr)->reload();
    }

    void calculatorPrefix() {
        const auto r = first(QStringLiteral("=2*(3+4)"));
        QCOMPARE(r.value(QStringLiteral("kind")).toString(), QStringLiteral("calc"));
        QCOMPARE(r.value(QStringLiteral("value")).toString(), QStringLiteral("14"));
    }

    void calculatorErrorIsShown() {
        const auto r = first(QStringLiteral("=1/0"));
        QCOMPARE(r.value(QStringLiteral("kind")).toString(), QStringLiteral("error"));
    }

    void mathIsDetectedWithoutPrefix() {
        const auto r = first(QStringLiteral("sqrt(16)+1"));
        QCOMPARE(r.value(QStringLiteral("kind")).toString(), QStringLiteral("calc"));
        QCOMPARE(r.value(QStringLiteral("value")).toString(), QStringLiteral("5"));
    }

    void timer() {
        const auto r = first(QStringLiteral("15 min pasta"));
        QCOMPARE(r.value(QStringLiteral("kind")).toString(), QStringLiteral("timer"));
        QCOMPARE(r.value(QStringLiteral("value")).toInt(), 900);
        QCOMPARE(r.value(QStringLiteral("label")).toString(), QStringLiteral("pasta"));
        search->activate(r);
        const auto timers = island::Timers::instance()->timers();
        QCOMPARE(timers.size(), 1);
        QCOMPARE(timers.first().toMap().value(QStringLiteral("label")).toString(), QStringLiteral("pasta"));
        island::Timers::instance()->remove(timers.first().toMap().value(QStringLiteral("id")).toInt());
    }

    void timerRingsAndStays() {
        auto* timers = island::Timers::instance();
        const int id = timers->start(1, QStringLiteral("tea"));
        QSignalSpy finished(timers, &island::Timers::finished);
        QVERIFY(finished.wait(3000));
        QTest::qWait(1500);
        const auto list = timers->timers();
        QCOMPARE(list.size(), 1);
        QVERIFY(list.first().toMap().value(QStringLiteral("done")).toBool());
        timers->remove(id);
    }

    void timerHint() {
        QCOMPARE(first(QStringLiteral("timer")).value(QStringLiteral("kind")).toString(),
                 QStringLiteral("error"));
    }

    void command() {
        const auto r = first(QStringLiteral("> echo hi"));
        QCOMPARE(r.value(QStringLiteral("kind")).toString(), QStringLiteral("command"));
        QCOMPARE(r.value(QStringLiteral("value")).toString(), QStringLiteral("echo hi"));
    }

    void web() {
        const auto r = first(QStringLiteral("?qt layer shell"));
        QCOMPARE(r.value(QStringLiteral("kind")).toString(), QStringLiteral("web"));
        QCOMPARE(r.value(QStringLiteral("value")).toString(), QStringLiteral("qt layer shell"));
        QCOMPARE(r.value(QStringLiteral("subtitle")).toString(), QStringLiteral("duckduckgo.com"));
    }

    void emoji() {
        const auto results = search->query(QStringLiteral(":fire"));
        QVERIFY(!results.isEmpty());
        const auto r = results.first().toMap();
        QCOMPARE(r.value(QStringLiteral("kind")).toString(), QStringLiteral("emoji"));
        QCOMPARE(r.value(QStringLiteral("value")).toString(), QStringLiteral("🔥"));
    }

    void emojiDataLoaded() {
        QVERIFY(search->query(QStringLiteral(":"), 5000).size() > 1500);
    }

    void plainTextFindsApps() {
        // Whatever is installed: every result for a plain word is an app
        // (no windows without Hyprland, no calculator for words).
        const auto results = search->query(QStringLiteral("a"));
        for (const auto& v : results)
            QCOMPARE(v.toMap().value(QStringLiteral("kind")).toString(), QStringLiteral("app"));
    }

    void newEvent() {
        const auto r = first(QStringLiteral("+ mañana 15:00 dentista"));
        QCOMPARE(r.value(QStringLiteral("kind")).toString(), QStringLiteral("event"));
        QCOMPARE(r.value(QStringLiteral("title")).toString(), QStringLiteral("dentista"));
        QVERIFY(r.value(QStringLiteral("subtitle")).toString().contains(QStringLiteral("Personal")));

        search->activate(r);
        const auto files = QDir(tmp.filePath(QStringLiteral("cal/personal")))
                               .entryList({QStringLiteral("*.ics")}, QDir::Files);
        QCOMPARE(files.size(), 1);
        auto* calendar = island::Calendar::create(nullptr, nullptr);
        const auto tomorrow = calendar->eventsOn(QDate::currentDate().addDays(1));
        QCOMPARE(tomorrow.size(), 1);
        QCOMPARE(tomorrow.first().toMap().value(QStringLiteral("summary")).toString(),
                 QStringLiteral("dentista"));
    }

    void newEventNeedsTitle() {
        QCOMPARE(first(QStringLiteral("+ mañana 15:00")).value(QStringLiteral("kind")).toString(),
                 QStringLiteral("error"));
    }

    void wordsAreNotMath() {
        QVERIFY(first(QStringLiteral("firefox")).value(QStringLiteral("kind")) != QStringLiteral("calc"));
    }
};

QTEST_MAIN(TestLauncherSearch)
#include "tst_launchersearch.moc"
