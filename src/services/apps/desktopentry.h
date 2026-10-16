#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <optional>

namespace island {

struct DesktopAction {
    QString id;
    QString name;
    QString exec;
    QString icon;
};

struct DesktopEntry {
    QString id; // e.g. "org.kde.dolphin.desktop"
    QString name;
    QString genericName;
    QString comment;
    QStringList keywords;
    QString exec;
    QString tryExec;
    QString icon;
    QString startupWmClass; // matches Wayland app_id / X11 class
    bool terminal = false;
    bool noDisplay = false;
    bool hidden = false;
    QStringList onlyShowIn;
    QStringList notShowIn;
    QList<DesktopAction> actions;

    // Whether it belongs in the launcher for `desktops` ($XDG_CURRENT_DESKTOP).
    [[nodiscard]] bool visibleIn(const QStringList& desktops) const;
};

// Parses a .desktop file. Localized keys are picked for `locale` ("es_CL"
// tries Name[es_CL], then Name[es], then Name). Returns nullopt for anything
// that is not a Type=Application entry.
std::optional<DesktopEntry> parseDesktopEntry(const QString& text, const QString& locale);

// Removes field codes (%f %F %u %U %i %c %k ...) and unescapes %%.
QString stripFieldCodes(const QString& exec);

// Quotes one argument for sh: it's -> 'it'\''s'.
QString shellQuote(const QString& arg);

// Fuzzy subsequence score of `query` in `text`; -1 when not all characters
// match in order. Rewards prefix, word-start and consecutive matches.
int fuzzyScore(QStringView query, QStringView text);

// Overall launcher score for an entry, or -1 when it does not match.
// `launches` (usage count) nudges matches up; with an empty query it is the
// whole score, so the most used apps come first.
int rankEntry(const DesktopEntry& entry, QStringView query, int launches);

} // namespace island
