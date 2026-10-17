#include "desktopentry.h"

#include <QHash>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>
#include <cmath>

using namespace Qt::StringLiterals;

namespace island {

bool DesktopEntry::visibleIn(const QStringList& desktops) const {
    if (noDisplay || hidden)
        return false;
    auto intersects = [&](const QStringList& list) {
        for (const auto& d : desktops)
            if (list.contains(d, Qt::CaseInsensitive))
                return true;
        return false;
    };
    if (!onlyShowIn.isEmpty() && !intersects(onlyShowIn))
        return false;
    if (!notShowIn.isEmpty() && intersects(notShowIn))
        return false;
    return true;
}

namespace {

using Group = QHash<QString, QString>;

QString unescape(const QString& value) {
    QString out;
    out.reserve(value.size());
    for (qsizetype i = 0; i < value.size(); ++i) {
        if (value[i] == u'\\' && i + 1 < value.size()) {
            const QChar c = value[++i];
            if (c == u's')
                out += u' ';
            else if (c == u'n')
                out += u'\n';
            else if (c == u't')
                out += u'\t';
            else if (c == u'r')
                out += u'\r';
            else
                out += c; // "\\" and "\;" keep the character
        } else {
            out += value[i];
        }
    }
    return out;
}

QStringList splitList(const QString& value) {
    QStringList out;
    QString current;
    for (qsizetype i = 0; i < value.size(); ++i) {
        if (value[i] == u'\\' && i + 1 < value.size() && value[i + 1] == u';') {
            current += u';';
            ++i;
        } else if (value[i] == u';') {
            if (!current.isEmpty())
                out.append(unescape(current));
            current.clear();
        } else {
            current += value[i];
        }
    }
    if (!current.isEmpty())
        out.append(unescape(current));
    return out;
}

QString localized(const Group& g, const QString& key, const QString& locale) {
    if (!locale.isEmpty()) {
        auto it = g.find(key + u'[' + locale + u']');
        if (it != g.end())
            return *it;
        const auto lang = locale.section(u'_', 0, 0);
        it = g.find(key + u'[' + lang + u']');
        if (it != g.end())
            return *it;
    }
    return g.value(key);
}

bool isTrue(const QString& v) {
    return v.compare(u"true", Qt::CaseInsensitive) == 0;
}

} // namespace

std::optional<DesktopEntry> parseDesktopEntry(const QString& text, const QString& locale) {
    QHash<QString, Group> groups;
    Group* current = nullptr;
    for (QStringView line : QStringTokenizer(text, u'\n')) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith(u'#'))
            continue;
        if (line.startsWith(u'[') && line.endsWith(u']')) {
            current = &groups[line.sliced(1, line.size() - 2).toString()];
            continue;
        }
        const auto eq = line.indexOf(u'=');
        if (!current || eq <= 0)
            continue;
        const auto key = line.first(eq).trimmed().toString();
        if (!current->contains(key))
            current->insert(key, line.sliced(eq + 1).trimmed().toString());
    }

    const auto it = groups.constFind(QStringLiteral("Desktop Entry"));
    if (it == groups.cend())
        return std::nullopt;
    const Group& g = *it;
    if (g.value(QStringLiteral("Type")) != u"Application")
        return std::nullopt;

    DesktopEntry e;
    e.name = unescape(localized(g, QStringLiteral("Name"), locale));
    if (e.name.isEmpty())
        return std::nullopt;
    e.genericName = unescape(localized(g, QStringLiteral("GenericName"), locale));
    e.comment = unescape(localized(g, QStringLiteral("Comment"), locale));
    e.keywords = splitList(localized(g, QStringLiteral("Keywords"), locale));
    e.exec = unescape(g.value(QStringLiteral("Exec")));
    e.tryExec = unescape(g.value(QStringLiteral("TryExec")));
    e.icon = unescape(g.value(QStringLiteral("Icon")));
    e.startupWmClass = g.value(QStringLiteral("StartupWMClass"));
    e.terminal = isTrue(g.value(QStringLiteral("Terminal")));
    e.noDisplay = isTrue(g.value(QStringLiteral("NoDisplay")));
    e.hidden = isTrue(g.value(QStringLiteral("Hidden")));
    e.onlyShowIn = splitList(g.value(QStringLiteral("OnlyShowIn")));
    e.notShowIn = splitList(g.value(QStringLiteral("NotShowIn")));

    for (const auto& id : splitList(g.value(QStringLiteral("Actions")))) {
        const auto ag = groups.constFind(QStringLiteral("Desktop Action ") + id);
        if (ag == groups.cend())
            continue;
        DesktopAction a{
            .id = id,
            .name = unescape(localized(*ag, QStringLiteral("Name"), locale)),
            .exec = unescape(ag->value(QStringLiteral("Exec"))),
            .icon = unescape(ag->value(QStringLiteral("Icon"))),
        };
        if (!a.name.isEmpty() && !a.exec.isEmpty())
            e.actions.append(a);
    }
    if (e.exec.isEmpty())
        return std::nullopt;
    return e;
}

QString stripFieldCodes(const QString& exec) {
    QString out;
    out.reserve(exec.size());
    for (qsizetype i = 0; i < exec.size(); ++i) {
        if (exec[i] == u'%' && i + 1 < exec.size()) {
            const QChar c = exec[++i];
            if (c == u'%')
                out += u'%';
            // Every other field code expands to nothing when launching
            // without files.
            continue;
        }
        out += exec[i];
    }
    return out.simplified();
}

int fuzzyScore(QStringView query, QStringView text) {
    if (query.isEmpty())
        return 0;
    int score = 0;
    qsizetype t = 0;
    qsizetype lastMatch = -2;
    for (qsizetype q = 0; q < query.size(); ++q) {
        const QChar qc = query[q].toLower();
        bool found = false;
        for (; t < text.size(); ++t) {
            if (text[t].toLower() != qc)
                continue;
            const bool wordStart = t == 0 || !text[t - 1].isLetterOrNumber() ||
                                   (text[t].isUpper() && text[t - 1].isLower());
            score += 1;
            if (t == 0)
                score += 15;
            else if (wordStart)
                score += 8;
            if (t == lastMatch + 1)
                score += 5;
            lastMatch = t;
            ++t;
            found = true;
            break;
        }
        if (!found)
            return -1;
    }
    // A literal match beats scattered letters ("fire" in "fire", not in
    // "flag: Ireland"); more so at the start of a word.
    const auto at = text.indexOf(query, 0, Qt::CaseInsensitive);
    if (at >= 0) {
        score += 30;
        if (at == 0 || !text[at - 1].isLetterOrNumber())
            score += 15;
    }
    // Shorter names win among equal matches.
    score -= static_cast<int>(std::min<qsizetype>(text.size(), 40) / 4);
    return std::max(score, 1);
}

QString shellQuote(const QString& arg) {
    QString out = arg;
    out.replace(u'\'', QStringLiteral("'\\''"));
    return u'\'' + out + u'\'';
}

int rankEntry(const DesktopEntry& entry, QStringView query, int launches) {
    // No query: most used first, by the plain count (a log would tie apps
    // with similar counts).
    if (query.isEmpty())
        return launches;
    const int usage = static_cast<int>(std::log2(1.0 + launches) * 3.0);
    int best = -1;
    auto consider = [&](QStringView text, int weight) {
        const int s = fuzzyScore(query, text);
        if (s >= 0)
            best = std::max(best, s * weight);
    };
    consider(entry.name, 3);
    consider(entry.genericName, 2);
    for (const auto& k : entry.keywords)
        consider(k, 2);
    const auto program = stripFieldCodes(entry.exec).section(u' ', 0, 0).section(u'/', -1);
    consider(program, 2);
    return best < 0 ? -1 : best + usage;
}

QString commandProgram(const QString& command) {
    static const QRegularExpression assignment(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*="));
    const auto words = command.split(u' ', Qt::SkipEmptyParts);
    for (const auto& word : words) {
        if (assignment.match(word).hasMatch() || word == u"env")
            continue;
        auto program = word;
        program.remove(u'"').remove(u'\'');
        return program.section(u'/', -1);
    }
    return {};
}

bool isGraphicalCommand(const QString& command, const QList<DesktopEntry>& entries) {
    static const QSet<QString> wrappers{u"flatpak"_s, u"sh"_s, u"bash"_s, u"zsh"_s, u"env"_s,
                                        u"gtk-launch"_s, u"kioclient"_s, u"kioclient5"_s,
                                        u"xdg-open"_s, u"snap"_s};
    const auto program = commandProgram(command);
    if (program.isEmpty())
        return false;
    return std::ranges::any_of(entries, [&](const DesktopEntry& e) {
        const auto runs = commandProgram(stripFieldCodes(e.exec));
        return !e.terminal && runs == program && !wrappers.contains(runs);
    });
}

} // namespace island
