#include "core/pywal.h"

#include <QTest>

using island::parsePywal;

class TestPywal : public QObject {
    Q_OBJECT

private slots:
    void parsesColors() {
        const auto colors = parsePywal(
            "{\"special\": {\"background\": \"#121a1b\", \"foreground\": \"#b3baba\"},"
            " \"colors\": {\"color0\": \"#121a1b\", \"color4\": \"#8C6D66\","
            " \"color15\": \"#ffffff\"}}");
        QVERIFY(colors);
        QCOMPARE(colors->background, QColor(0x12, 0x1a, 0x1b));
        QCOMPARE(colors->named(QStringLiteral("color4")), QColor(0x8c, 0x6d, 0x66));
        QCOMPARE(colors->named(QStringLiteral("color15")), QColor(Qt::white));
        QVERIFY(!colors->named(QStringLiteral("color16")).isValid());
        QVERIFY(!colors->named(QStringLiteral("accent")).isValid());
    }

    void rejectsIncomplete() {
        QVERIFY(!parsePywal("{}"));
        QVERIFY(!parsePywal("nope"));
    }
};

QTEST_APPLESS_MAIN(TestPywal)
#include "tst_pywal.moc"
