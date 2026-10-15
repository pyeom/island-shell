#include "services/launcher/calc.h"

#include <QTest>

#include <cmath>

using namespace island;

class TestCalc : public QObject {
    Q_OBJECT

    static double eval(const char* text) {
        QString error;
        const auto v = evaluate(QString::fromUtf8(text), &error);
        if (!v)
            qWarning() << text << error;
        return v.value_or(NAN);
    }

private slots:
    void arithmetic() {
        QCOMPARE(eval("2+3*4"), 14.0);
        QCOMPARE(eval("(2+3)*4"), 20.0);
        QCOMPARE(eval("10/4"), 2.5);
        QCOMPARE(eval("10 % 4"), 2.0);
        QCOMPARE(eval("2^3^2"), 512.0); // right associative
        QCOMPARE(eval("-2^2"), -4.0);
        QCOMPARE(eval("--3"), 3.0);
        QCOMPARE(eval("6 × 7 ÷ 2"), 21.0);
    }

    void decimalComma() {
        QCOMPARE(eval("2,5*2"), 5.0);
        QCOMPARE(eval("0.5+0,25"), 0.75);
    }

    void functionsAndConstants() {
        QCOMPARE(eval("sqrt(16)"), 4.0);
        QCOMPARE(eval("abs(-3)"), 3.0);
        QCOMPARE(eval("log(1000)"), 3.0);
        QVERIFY(std::abs(eval("sin(pi/2)") - 1.0) < 1e-12);
        QVERIFY(std::abs(eval("2pi") - 2 * M_PI) < 1e-12);
        QCOMPARE(eval("3(4+1)"), 15.0);
        QVERIFY(std::abs(eval("ln(e)") - 1.0) < 1e-12);
    }

    void errors() {
        QString error;
        QVERIFY(!evaluate(u"1/0", &error));
        QVERIFY(error.contains(u"zero"));
        QVERIFY(!evaluate(u"2+", &error));
        QVERIFY(!evaluate(u"(1+2", &error));
        QVERIFY(!evaluate(u"foo(2)", &error));
        QVERIFY(!evaluate(u"sqrt 2", &error));
        QVERIFY(!evaluate(u"", &error));
        QVERIFY(!evaluate(u"sqrt(-1)", &error)); // NaN is not a result
    }

    void detection() {
        QVERIFY(looksLikeMath(u"2+2"));
        QVERIFY(looksLikeMath(u"sqrt(2)"));
        QVERIFY(looksLikeMath(u"2pi"));
        QVERIFY(!looksLikeMath(u"firefox"));
        QVERIFY(!looksLikeMath(u"42"));          // a number alone is not a question
        QVERIFY(!looksLikeMath(u"top 10 games")); // words
        QVERIFY(!looksLikeMath(u"7zip"));
    }

    void formatting() {
        QCOMPARE(formatNumber(0.1 + 0.2), QStringLiteral("0.3"));
        QCOMPARE(formatNumber(14), QStringLiteral("14"));
        QCOMPARE(formatNumber(-0.0), QStringLiteral("0"));
        QCOMPARE(formatNumber(1.0 / 3), QStringLiteral("0.333333333333"));
        QCOMPARE(formatNumber(2.5), QStringLiteral("2.5"));
        QCOMPARE(formatNumber(1e20), QStringLiteral("1e+20"));
    }
};

QTEST_APPLESS_MAIN(TestCalc)
#include "tst_calc.moc"
