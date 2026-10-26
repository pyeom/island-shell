#include "timerparse.h"

#include <QStringList>

namespace island {

using namespace Qt::StringLiterals;

namespace {

constexpr int kMaxSeconds = 7 * 24 * 3600;

// Seconds per unit, or 0 for an unknown one.
int unitSeconds(QStringView unit) {
    static const QStringList hours = {u"h"_s, u"hr"_s, u"hrs"_s, u"hour"_s, u"hours"_s, u"hora"_s,
                                      u"horas"_s};
    static const QStringList minutes = {u"m"_s, u"min"_s, u"mins"_s, u"minute"_s, u"minutes"_s,
                                        u"minuto"_s, u"minutos"_s, u"'"_s, u"’"_s};
    static const QStringList seconds = {u"s"_s, u"sec"_s, u"secs"_s, u"second"_s, u"seconds"_s,
                                        u"seg"_s, u"segs"_s, u"segundo"_s, u"segundos"_s,
                                        u"''"_s, u"’’"_s, u"\""_s, u"”"_s};
    const auto u = unit.toString().toLower();
    if (hours.contains(u))
        return 3600;
    if (minutes.contains(u))
        return 60;
    if (seconds.contains(u))
        return 1;
    return 0;
}

bool isQuote(QChar c) {
    return c == u'\'' || c == u'"' || c == u'’' || c == u'”';
}

struct Cursor {
    QStringView s;
    qsizetype i = 0;

    bool atEnd() const { return i >= s.size(); }
    void skipSpaces() {
        while (!atEnd() && s[i].isSpace())
            ++i;
    }
    // 15, 1.5, 1,5
    std::optional<double> number() {
        const auto start = i;
        while (!atEnd() && s[i].isDigit())
            ++i;
        if (i == start)
            return std::nullopt;
        if (i + 1 < s.size() && (s[i] == u'.' || s[i] == u',') && s[i + 1].isDigit()) {
            ++i;
            while (!atEnd() && s[i].isDigit())
                ++i;
        }
        auto text = s.mid(start, i - start).toString();
        text.replace(u',', u'.');
        return text.toDouble();
    }
    // A word of letters, or a run of quote marks (' '' ").
    QStringView unit() {
        const auto start = i;
        if (!atEnd() && isQuote(s[i])) {
            while (!atEnd() && isQuote(s[i]))
                ++i;
        } else {
            while (!atEnd() && s[i].isLetter())
                ++i;
        }
        return s.mid(start, i - start);
    }
};

} // namespace

std::optional<TimerRequest> parseTimer(QStringView text) {
    Cursor c{text.trimmed()};
    bool prefixed = false;
    for (const auto prefix : {u"timer"_s, u"temporizador"_s}) {
        if (c.s.startsWith(prefix, Qt::CaseInsensitive) &&
            (c.s.size() == prefix.size() || c.s[prefix.size()].isSpace())) {
            c.i = prefix.size();
            prefixed = true;
            break;
        }
    }

    double total = 0;
    int parts = 0;
    int lastUnit = 0;
    qsizetype end = c.i; // after the last part read
    while (true) {
        c.skipSpaces();
        const auto before = c.i;
        const auto n = c.number();
        if (!n)
            break;
        const auto afterNumber = c.i;
        c.skipSpaces();
        const auto unitText = c.unit();
        // Whole words only: "15 minecraft" reads "minecraft", not "min".
        int unit = unitSeconds(unitText);
        if (!unit) {
            // "1h30": a bare number after a unit is the next smaller one;
            // "timer 5": minutes.
            if (lastUnit == 3600 || lastUnit == 60)
                unit = lastUnit / 60;
            else if (prefixed && parts == 0)
                unit = 60;
            else {
                c.i = before;
                break;
            }
            c.i = afterNumber;
        }
        total += *n * unit;
        lastUnit = unit;
        ++parts;
        end = c.i;
    }
    c.i = end;
    if (parts == 0 || total < 1 || total > kMaxSeconds)
        return std::nullopt;
    // The label must be separated from the duration: "15mx" is not a timer.
    if (!c.atEnd() && !c.s[c.i].isSpace())
        return std::nullopt;
    return TimerRequest{static_cast<int>(total + 0.5), c.s.mid(c.i).trimmed().toString()};
}

QString formatDuration(int seconds) {
    const int h = seconds / 3600;
    const int m = seconds % 3600 / 60;
    const int s = seconds % 60;
    QStringList parts;
    if (h)
        parts << QStringLiteral("%1 h").arg(h);
    if (m)
        parts << QStringLiteral("%1 min").arg(m);
    if (s || parts.isEmpty())
        parts << QStringLiteral("%1 s").arg(s);
    return parts.join(u' ');
}

} // namespace island
