#include "launchersearch.h"

#include "apps.h"
#include "calc.h"
#include "config.h"
#include "desktopentry.h"
#include "hyprland.h"
#include "hyprlandparse.h"
#include "icons.h"

#include <QCoreApplication>
#include <QFile>
#include <QJSEngine>
#include <QUrl>

#include <algorithm>

namespace island {

namespace {

QVariantMap item(const char* kind, const QString& title, const QString& subtitle,
                 const QString& icon, const QVariant& value) {
    return {
        {QStringLiteral("kind"), QLatin1String(kind)},
        {QStringLiteral("title"), title},
        {QStringLiteral("subtitle"), subtitle},
        {QStringLiteral("icon"), icon},
        {QStringLiteral("value"), value},
    };
}

QVariantMap calcItem(const QString& expression, bool explicitMode) {
    QString error;
    const auto result = evaluate(expression, &error);
    if (!result) {
        if (!explicitMode)
            return {};
        return item("error", error, QCoreApplication::translate("LauncherSearch", "Calculator"),
                    Icons::source(QStringLiteral("accessories-calculator")), {});
    }
    const auto text = formatNumber(*result);
    return item("calc", QStringLiteral("= ") + text,
                expression.trimmed() + QStringLiteral("  ·  ") +
                    QCoreApplication::translate("LauncherSearch", "Enter copies the result"),
                Icons::source(QStringLiteral("accessories-calculator"),
                              QStringLiteral("applications-utilities")),
                text);
}

} // namespace

LauncherSearch* LauncherSearch::create(QQmlEngine*, QJSEngine*) {
    static auto* search = new LauncherSearch(QCoreApplication::instance());
    QJSEngine::setObjectOwnership(search, QJSEngine::CppOwnership);
    return search;
}

LauncherSearch::LauncherSearch(QObject* parent) : QObject(parent) {
    loadEmoji();
}

void LauncherSearch::loadEmoji() {
    QFile f(QStringLiteral(":/qt/qml/IslandShell/emoji.tsv"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    while (!f.atEnd()) {
        const auto line = QString::fromUtf8(f.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(u'#'))
            continue;
        const auto parts = line.split(u'\t');
        if (parts.size() >= 2)
            m_emoji.append({parts[0], parts[1], parts.value(2)});
    }
}

void LauncherSearch::refresh() {
    Hyprland::instance()->request("j/clients", [this](const QByteArray& json) {
        m_windows = hypr::parseClients(json);
        ++m_revision;
        emit changed();
    });
}

QVariantList LauncherSearch::windowResults(const QString& query, int limit) const {
    struct Hit {
        int score;
        QVariantMap window;
    };
    std::vector<Hit> hits;
    for (const auto& v : m_windows) {
        const auto w = v.toMap();
        const int score = std::max(fuzzyScore(query, w.value(QStringLiteral("title")).toString()),
                                   fuzzyScore(query, w.value(QStringLiteral("class")).toString()) * 2);
        if (score > 0)
            hits.push_back({score, w});
    }
    std::ranges::stable_sort(hits, std::greater{}, &Hit::score);
    QVariantList out;
    for (const auto& h : hits) {
        if (out.size() >= limit)
            break;
        const auto cls = h.window.value(QStringLiteral("class")).toString();
        out.append(item("window", h.window.value(QStringLiteral("title")).toString(),
                        QCoreApplication::translate("LauncherSearch", "Window · %1 · workspace %2")
                            .arg(cls, h.window.value(QStringLiteral("workspace")).toString()),
                        Apps::instance()->iconForClass(cls),
                        h.window.value(QStringLiteral("address"))));
    }
    return out;
}

QVariantList LauncherSearch::emojiResults(const QString& query, int limit) const {
    struct Hit {
        int score;
        const Emoji* emoji;
    };
    std::vector<Hit> hits;
    for (const auto& e : m_emoji) {
        const int score = query.isEmpty()
                              ? 1
                              : std::max(fuzzyScore(query, e.name) * 3, fuzzyScore(query, e.keywords));
        if (score > 0)
            hits.push_back({score, &e});
    }
    std::ranges::stable_sort(hits, std::greater{}, &Hit::score);
    QVariantList out;
    for (const auto& h : hits) {
        if (out.size() >= limit)
            break;
        auto it = item("emoji", h.emoji->name, h.emoji->keywords, {}, h.emoji->emoji);
        it.insert(QStringLiteral("glyph"), h.emoji->emoji);
        out.append(it);
    }
    return out;
}

QVariantList LauncherSearch::query(const QString& text, int limit) const {
    const auto t = text.trimmed();
    const auto rest = t.mid(1).trimmed();

    if (t.startsWith(u'=')) {
        const auto calc = calcItem(rest, true);
        return rest.isEmpty() ? QVariantList() : QVariantList{calc};
    }
    if (t.startsWith(u'>')) {
        if (rest.isEmpty())
            return {item("error", QCoreApplication::translate("LauncherSearch", "Type a command"),
                         QCoreApplication::translate("LauncherSearch", "Enter runs it, Shift+Enter in a terminal"),
                         Icons::source(QStringLiteral("utilities-terminal")), {})};
        return {item("command", rest,
                     QCoreApplication::translate("LauncherSearch", "Run command · Shift+Enter: in a terminal"),
                     Icons::source(QStringLiteral("utilities-terminal")), rest)};
    }
    if (t.startsWith(u'?')) {
        if (rest.isEmpty())
            return {};
        return {item("web", QCoreApplication::translate("LauncherSearch", "Search the web for “%1”").arg(rest),
                     QUrl(QString::fromStdString(Config::instance()->data().launcher.searchUrl)).host(),
                     Icons::source(QStringLiteral("internet-web-browser"), QStringLiteral("applications-internet")),
                     rest)};
    }
    if (t.startsWith(u':'))
        return emojiResults(rest, limit);

    QVariantList out;
    if (looksLikeMath(t)) {
        const auto calc = calcItem(t, false);
        if (!calc.isEmpty())
            out.append(calc);
    }
    if (!t.isEmpty())
        out.append(windowResults(t, 4));
    for (const auto& v : Apps::instance()->search(t, limit)) {
        auto app = v.toMap();
        auto it = item("app", app.value(QStringLiteral("name")).toString(),
                       app.value(QStringLiteral("genericName")).toString().isEmpty()
                           ? app.value(QStringLiteral("comment")).toString()
                           : app.value(QStringLiteral("genericName")).toString(),
                       app.value(QStringLiteral("icon")).toString(), app.value(QStringLiteral("id")));
        it.insert(QStringLiteral("actions"), app.value(QStringLiteral("actions")));
        out.append(it);
    }
    while (out.size() > limit)
        out.removeLast();
    return out;
}

void LauncherSearch::activate(const QVariantMap& it, int action) {
    const auto kind = it.value(QStringLiteral("kind")).toString();
    const auto value = it.value(QStringLiteral("value")).toString();
    auto* apps = Apps::instance();
    if (kind == u"app") {
        const auto actions = it.value(QStringLiteral("actions")).toList();
        const auto actionId = action >= 0 && action < actions.size()
                                  ? actions[action].toMap().value(QStringLiteral("id")).toString()
                                  : QString();
        apps->launch(value, actionId);
    } else if (kind == u"window") {
        Hyprland::instance()->dispatch(QStringLiteral("focuswindow address:") + value);
    } else if (kind == u"calc" || kind == u"emoji") {
        apps->copyText(value);
    } else if (kind == u"command") {
        if (action == 1)
            apps->runInTerminal(value);
        else
            apps->run(value);
    } else if (kind == u"web") {
        auto url = QString::fromStdString(Config::instance()->data().launcher.searchUrl);
        url.replace(QStringLiteral("%s"), QString::fromUtf8(QUrl::toPercentEncoding(value)));
        apps->openUrl(url);
    }
}

} // namespace island
