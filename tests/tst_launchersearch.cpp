// Integration test: LauncherSearch against the module's real services
// (Apps scans the system's .desktop files; Hyprland is disabled).
#include "launchersearch.h"

#include <QGuiApplication>
#include <QTest>

using island::LauncherSearch;

class TestLauncherSearch : public QObject {
    Q_OBJECT

    LauncherSearch* search = nullptr;

    QVariantMap first(const QString& query) {
        const auto results = search->query(query);
        return results.isEmpty() ? QVariantMap() : results.first().toMap();
    }

private slots:
    void initTestCase() {
        qunsetenv("HYPRLAND_INSTANCE_SIGNATURE"); // never talk to the real compositor
        qputenv("ISLAND_SHELL_CONFIG", "/nonexistent/config.toml");
        search = LauncherSearch::create(nullptr, nullptr);
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

    void wordsAreNotMath() {
        QVERIFY(first(QStringLiteral("firefox")).value(QStringLiteral("kind")) != QStringLiteral("calc"));
    }
};

QTEST_MAIN(TestLauncherSearch)
#include "tst_launchersearch.moc"
