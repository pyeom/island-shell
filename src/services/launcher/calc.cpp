#include "calc.h"

#include <QHash>

#include <cmath>
#include <functional>
#include <numbers>

namespace island {

namespace {

class Parser {
public:
    explicit Parser(QStringView text) : m_text(text) {}

    std::optional<double> parse(QString* error) {
        try {
            const double v = expression();
            skipSpace();
            if (m_pos != m_text.size())
                fail(QStringLiteral("unexpected \"%1\"").arg(m_text[m_pos]));
            if (!std::isfinite(v))
                fail(QStringLiteral("result is not a finite number"));
            return v;
        } catch (const QString& e) {
            if (error)
                *error = e;
            return std::nullopt;
        }
    }

private:
    [[noreturn]] void fail(const QString& message) { throw message; }

    void skipSpace() {
        while (m_pos < m_text.size() && m_text[m_pos].isSpace())
            ++m_pos;
    }

    bool accept(QChar c) {
        skipSpace();
        if (m_pos < m_text.size() && m_text[m_pos] == c) {
            ++m_pos;
            return true;
        }
        return false;
    }

    QChar peek() {
        skipSpace();
        return m_pos < m_text.size() ? m_text[m_pos] : QChar();
    }

    double expression() {
        double v = term();
        for (;;) {
            if (accept(u'+'))
                v += term();
            else if (accept(u'-') || accept(u'−'))
                v -= term();
            else
                return v;
        }
    }

    double term() {
        double v = power();
        for (;;) {
            if (accept(u'*') || accept(u'×')) {
                v *= power();
            } else if (accept(u'/') || accept(u'÷')) {
                const double d = power();
                if (d == 0)
                    fail(QStringLiteral("division by zero"));
                v /= d;
            } else if (accept(u'%')) {
                const double d = power();
                if (d == 0)
                    fail(QStringLiteral("division by zero"));
                v = std::fmod(v, d);
            } else if (startsOperand()) {
                v *= power(); // implicit: 2pi, 3(4+1)
            } else {
                return v;
            }
        }
    }

    // Unary minus binds looser than ^: -2^2 = -4.
    double power() {
        if (accept(u'-') || accept(u'−'))
            return -power();
        if (accept(u'+'))
            return power();
        const double base = primary();
        if (accept(u'^'))
            return std::pow(base, power());
        return base;
    }

    bool startsOperand() {
        const QChar c = peek();
        return c == u'(' || c.isLetter() || c == u'π';
    }

    double primary() {
        skipSpace();
        if (m_pos >= m_text.size())
            fail(QStringLiteral("incomplete expression"));
        const QChar c = m_text[m_pos];
        if (accept(u'(')) {
            const double v = expression();
            if (!accept(u')'))
                fail(QStringLiteral("missing \")\""));
            return v;
        }
        if (c.isDigit() || c == u'.' || c == u',')
            return number();
        if (c == u'π') {
            ++m_pos;
            return std::numbers::pi;
        }
        if (c.isLetter())
            return identifier();
        fail(QStringLiteral("unexpected \"%1\"").arg(c));
    }

    double number() {
        QString digits;
        bool separator = false;
        while (m_pos < m_text.size()) {
            const QChar c = m_text[m_pos];
            if (c.isDigit()) {
                digits += c;
            } else if ((c == u'.' || c == u',') && !separator) {
                digits += u'.';
                separator = true;
            } else {
                break;
            }
            ++m_pos;
        }
        bool ok = false;
        const double v = digits.toDouble(&ok);
        if (!ok)
            fail(QStringLiteral("bad number \"%1\"").arg(digits));
        return v;
    }

    double identifier() {
        const qsizetype start = m_pos;
        while (m_pos < m_text.size() && m_text[m_pos].isLetter())
            ++m_pos;
        const QString name = m_text.sliced(start, m_pos - start).toString().toLower();
        if (name == u"pi")
            return std::numbers::pi;
        if (name == u"e")
            return std::numbers::e;

        using Fn = double (*)(double);
        static const QHash<QString, Fn> functions = {
            {QStringLiteral("sqrt"), [](double x) { return std::sqrt(x); }},
            {QStringLiteral("sin"), [](double x) { return std::sin(x); }},
            {QStringLiteral("cos"), [](double x) { return std::cos(x); }},
            {QStringLiteral("tan"), [](double x) { return std::tan(x); }},
            {QStringLiteral("asin"), [](double x) { return std::asin(x); }},
            {QStringLiteral("acos"), [](double x) { return std::acos(x); }},
            {QStringLiteral("atan"), [](double x) { return std::atan(x); }},
            {QStringLiteral("ln"), [](double x) { return std::log(x); }},
            {QStringLiteral("log"), [](double x) { return std::log10(x); }},
            {QStringLiteral("exp"), [](double x) { return std::exp(x); }},
            {QStringLiteral("abs"), [](double x) { return std::abs(x); }},
            {QStringLiteral("round"), [](double x) { return std::round(x); }},
            {QStringLiteral("floor"), [](double x) { return std::floor(x); }},
            {QStringLiteral("ceil"), [](double x) { return std::ceil(x); }},
        };
        const auto fn = functions.value(name);
        if (!fn)
            fail(QStringLiteral("unknown name \"%1\"").arg(name));
        if (!accept(u'('))
            fail(QStringLiteral("%1 needs parentheses").arg(name));
        const double arg = expression();
        if (!accept(u')'))
            fail(QStringLiteral("missing \")\""));
        return fn(arg);
    }

    QStringView m_text;
    qsizetype m_pos = 0;
};

} // namespace

std::optional<double> evaluate(QStringView expression, QString* error) {
    if (expression.trimmed().isEmpty()) {
        if (error)
            *error = QStringLiteral("empty expression");
        return std::nullopt;
    }
    return Parser(expression).parse(error);
}

bool looksLikeMath(QStringView text) {
    text = text.trimmed();
    bool digit = false;
    bool operation = false;
    for (const QChar c : text) {
        if (c.isDigit()) {
            digit = true;
        } else if (QStringView(u"+-*/%^()×÷−").contains(c)) {
            operation = true;
        } else if (!(c.isSpace() || c == u'.' || c == u',' || c.isLetter() || c == u'π')) {
            return false;
        }
    }
    // Letters are fine only as function or constant names.
    static const QStringList words = {
        QStringLiteral("sqrt"), QStringLiteral("sin"), QStringLiteral("cos"), QStringLiteral("tan"),
        QStringLiteral("asin"), QStringLiteral("acos"), QStringLiteral("atan"), QStringLiteral("ln"),
        QStringLiteral("log"), QStringLiteral("exp"), QStringLiteral("abs"), QStringLiteral("round"),
        QStringLiteral("floor"), QStringLiteral("ceil"), QStringLiteral("pi"), QStringLiteral("e")};
    qsizetype i = 0;
    while (i < text.size()) {
        if (!text[i].isLetter()) {
            ++i;
            continue;
        }
        qsizetype j = i;
        while (j < text.size() && text[j].isLetter())
            ++j;
        if (!words.contains(text.sliced(i, j - i).toString().toLower()))
            return false;
        operation = true;
        i = j;
    }
    return digit && operation;
}

QString formatNumber(double value) {
    if (value == 0)
        return QStringLiteral("0"); // also turns -0 into 0
    // Round to 12 significant digits first: 0.1 + 0.2 shows as 0.3.
    const double rounded = QString::number(value, 'g', 12).toDouble();
    const double magnitude = std::abs(rounded);
    if (magnitude >= 1e15 || magnitude < 1e-9)
        return QString::number(rounded, 'g', 12);
    const int integerDigits = static_cast<int>(std::floor(std::log10(magnitude))) + 1;
    QString s = QString::number(rounded, 'f', std::max(0, 12 - integerDigits));
    if (s.contains(u'.')) {
        while (s.endsWith(u'0'))
            s.chop(1);
        if (s.endsWith(u'.'))
            s.chop(1);
    }
    return s;
}

} // namespace island
