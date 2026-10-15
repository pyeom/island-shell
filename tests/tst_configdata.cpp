#include "core/configdata.h"

#include <QTest>

using namespace island;

class TestConfigData : public QObject {
    Q_OBJECT

private slots:
    void emptyGivesDefaults() {
        std::vector<std::string> errors;
        const auto cfg = parseConfig("", errors);
        QVERIFY(errors.empty());
        QCOMPARE(cfg.island.compactWidth, 190);
        QCOMPARE(cfg.island.exclusiveZone, 44);
        QVERIFY(cfg.animations == AnimationMode::Full);
        QCOMPARE(cfg.theme.style, std::string("black"));
    }

    void readsValues() {
        std::vector<std::string> errors;
        const auto cfg = parseConfig(R"(
[island]
compact_width = 220
exclusive_zone = 50

[animations]
mode = "reduced"

[theme]
source = "pywal"
style = "blur"
blur_opacity = 0.4

[clock]
format = "hh:mm AP"
)",
                                     errors);
        QVERIFY2(errors.empty(), errors.empty() ? "" : errors.front().c_str());
        QCOMPARE(cfg.island.compactWidth, 220);
        QCOMPARE(cfg.island.exclusiveZone, 50);
        QVERIFY(cfg.animations == AnimationMode::Reduced);
        QCOMPARE(cfg.theme.source, std::string("pywal"));
        QCOMPARE(cfg.theme.style, std::string("blur"));
        QCOMPARE(cfg.theme.blurOpacity, 0.4);
        QCOMPARE(cfg.clock.format, std::string("hh:mm AP"));
    }

    void syntaxErrorKeepsDefaults() {
        std::vector<std::string> errors;
        const auto cfg = parseConfig("[island\ncompact_width = 3", errors);
        QCOMPARE(errors.size(), size_t(1));
        QVERIFY(errors.front().starts_with("line 1"));
        QCOMPARE(cfg.island.compactWidth, 190);
    }

    void invalidValuesAreReported() {
        std::vector<std::string> errors;
        const auto cfg = parseConfig(R"(
[island]
compact_width = "wide"
compact_height = 5000
[animations]
mode = "bouncy"
)",
                                     errors);
        QCOMPARE(errors.size(), size_t(3));
        QCOMPARE(cfg.island.compactWidth, 190);
        QCOMPARE(cfg.island.compactHeight, 36);
        QVERIFY(cfg.animations == AnimationMode::Full);
    }

    // The surface never resizes during animation, so it must hold the
    // largest island state.
    void surfaceGrowsToFitIsland() {
        std::vector<std::string> errors;
        const auto cfg = parseConfig(R"(
[island]
expanded_width = 800
surface_width = 600
)",
                                     errors);
        QCOMPARE(errors.size(), size_t(1));
        QCOMPARE(cfg.island.surfaceWidth, 800);
    }

    void rawKeepsUnknownSections() {
        std::vector<std::string> errors;
        const auto cfg = parseConfig("[activities.weather]\ncity = \"Santiago\"\n", errors);
        QVERIFY(errors.empty());
        QCOMPARE(cfg.raw.at_path("activities.weather.city").value_or(std::string()),
                 std::string("Santiago"));
    }
};

QTEST_APPLESS_MAIN(TestConfigData)
#include "tst_configdata.moc"
