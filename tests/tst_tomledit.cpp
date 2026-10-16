#include "core/configdata.h"
#include "core/tomledit.h"

#include <QTest>

using namespace island;

namespace {
const QString kDoc = QStringLiteral(
    "# island-shell configuration\n"
    "\n"
    "[island]\n"
    "# Pill size.\n"
    "compact_width = 190   # pixels\n"
    "compact_height = 36\n"
    "\n"
    "[theme]\n"
    "source = \"static\"\n"
    "# style = \"blur\"\n"
    "accent = \"#0a84ff\"  # iOS blue\n"
    "\n"
    "[notifications]\n"
    "rules = [\n"
    "  { app = \"discord\", mute = true },\n"
    "]\n");
}

class TestTomlEdit : public QObject {
    Q_OBJECT

private slots:
    void literals() {
        QCOMPARE(tomlLiteral(true), QStringLiteral("true"));
        QCOMPARE(tomlLiteral(42), QStringLiteral("42"));
        QCOMPARE(tomlLiteral(0.5), QStringLiteral("0.5"));
        QCOMPARE(tomlLiteral(2.0), QStringLiteral("2.0"));
        QCOMPARE(tomlLiteral(QStringLiteral("say \"hi\"")), QStringLiteral("\"say \\\"hi\\\"\""));
        QCOMPARE(tomlLiteral(QStringList{QStringLiteral("~/a"), QStringLiteral("b")}),
                 QStringLiteral("[\"~/a\", \"b\"]"));
    }

    void replacesValueKeepsComments() {
        const auto out = tomlSet(kDoc, QStringLiteral("island"), QStringLiteral("compact_width"), 220);
        QVERIFY(out.contains(QStringLiteral("compact_width = 220  # pixels")));
        QVERIFY(out.contains(QStringLiteral("# Pill size.")));
        QCOMPARE(out.count(u'\n'), kDoc.count(u'\n'));
    }

    void commentWithHashInsideString() {
        const auto out = tomlSet(kDoc, QStringLiteral("theme"), QStringLiteral("accent"), QStringLiteral("#ff9f0a"));
        QVERIFY(out.contains(QStringLiteral("accent = \"#ff9f0a\"  # iOS blue")));
    }

    void commentedKeyIsLeftAlone() {
        const auto out = tomlSet(kDoc, QStringLiteral("theme"), QStringLiteral("style"), QStringLiteral("blur"));
        QVERIFY(out.contains(QStringLiteral("# style = \"blur\"")));
        QVERIFY(out.contains(QStringLiteral("accent = \"#0a84ff\"  # iOS blue\nstyle = \"blur\"")));
    }

    void newSection() {
        const auto out = tomlSet(kDoc, QStringLiteral("weather"), QStringLiteral("city"), QStringLiteral("Santiago"));
        QVERIFY(out.endsWith(QStringLiteral("\n[weather]\ncity = \"Santiago\"\n")));
        QVERIFY(out.contains(QStringLiteral("rules = [")));
    }

    void sameKeyNameInOtherSectionUntouched() {
        const QString doc = QStringLiteral("[a]\nx = 1\n[b]\nx = 2\n");
        QCOMPARE(tomlSet(doc, QStringLiteral("b"), QStringLiteral("x"), 5),
                 QStringLiteral("[a]\nx = 1\n[b]\nx = 5\n"));
    }

    void emptyDocument() {
        QCOMPARE(tomlSet(QString(), QStringLiteral("clock"), QStringLiteral("format"), QStringLiteral("h:mm AP")),
                 QStringLiteral("\n[clock]\nformat = \"h:mm AP\"\n"));
    }

    // Whatever we write still parses, with the new values.
    void resultParses() {
        auto out = tomlSet(kDoc, QStringLiteral("island"), QStringLiteral("compact_width"), 220);
        out = tomlSet(out, QStringLiteral("theme"), QStringLiteral("style"), QStringLiteral("blur"));
        out = tomlSet(out, QStringLiteral("calendar"), QStringLiteral("paths"),
                      QStringList{QStringLiteral("~/cal")});
        std::vector<std::string> errors;
        const auto cfg = parseConfig(out.toStdString(), errors);
        QVERIFY2(errors.empty(), errors.empty() ? "" : errors.front().c_str());
        QCOMPARE(cfg.island.compactWidth, 220);
        QCOMPARE(cfg.theme.style, std::string("blur"));
        QCOMPARE(cfg.calendar.paths.front(), std::string("~/cal"));
        QCOMPARE(cfg.notifications.rules.size(), size_t(1));
    }
};

QTEST_APPLESS_MAIN(TestTomlEdit)
#include "tst_tomledit.moc"
