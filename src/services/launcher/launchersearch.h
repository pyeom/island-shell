#pragma once

#include <QList>
#include <QVariantMap>

#include <functional>
#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// Everything the launcher can find, by query:
//
//   firefox      apps and open windows (plus a calculator result when the
//                query looks like math: 2*(3+4), sqrt(2), 2pi)
//   =expr        calculator
//   >command     run a shell command (Shift+Enter: in a terminal)
//   ?words       search the web ([launcher] search_engine)
//   :name        emoji
//   +text        new calendar event ("+ mañana 15:00 dentista")
//
// Items are maps: { kind, title, subtitle, icon, value, actions? } with kind
// one of app, window, calc, command, web, emoji, event, error.
class LauncherSearch : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // Bumped when the window list refreshes, so bindings re-query.
    Q_PROPERTY(int revision READ revision NOTIFY changed)

public:
    static LauncherSearch* create(QQmlEngine*, QJSEngine*);

    int revision() const { return m_revision; }

    // Re-reads open windows from Hyprland (call when the launcher opens).
    Q_INVOKABLE void refresh();
    Q_INVOKABLE QVariantList query(const QString& text, int limit = 30) const;
    // `action`: app desktop action index, or 1 for "in a terminal" on
    // commands; -1 for the default.
    Q_INVOKABLE void activate(const QVariantMap& item, int action = -1);

signals:
    void changed();

private:
    struct Emoji {
        QString emoji;
        QString name;
        QString keywords;
    };

    explicit LauncherSearch(QObject* parent = nullptr);
    void loadEmoji();
    QVariantList emojiResults(const QString& query, int limit) const;
    QVariantList windowResults(const QString& query, int limit) const;

    QList<Emoji> m_emoji;
    std::function<QVariantMap(const QString&)> m_calendarPreview;
    std::function<void(const QString&)> m_calendarAdd;
    QVariantList m_windows;
    int m_revision = 0;
};

} // namespace island
