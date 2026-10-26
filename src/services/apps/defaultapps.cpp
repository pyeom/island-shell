#include "defaultapps.h"

#include "apps.h"
#include "icons.h"

#include <QCoreApplication>
#include <QJSEngine>
#include <QProcess>
#include <QStandardPaths>

#include <algorithm>

namespace island {

namespace {

struct Category {
    const char* key;
    QStringList mimeTypes; // the first one is queried for the current default
    // Apps in all of these menu categories are offered too, even when they
    // do not declare the MIME types (VS Code only declares its workspaces).
    QStringList menuCategories;
};

const QList<Category>& categoryList() {
    static const QList<Category> list = {
        {"browser",
         {"x-scheme-handler/https", "x-scheme-handler/http", "text/html", "application/xhtml+xml"},
         {"WebBrowser"}},
        {"files", {"inode/directory"}, {"FileManager"}},
        {"mail", {"x-scheme-handler/mailto"}, {"Email"}},
        {"text", {"text/plain", "text/markdown", "application/x-shellscript"}, {"TextEditor"}},
        {"images",
         {"image/png", "image/jpeg", "image/gif", "image/webp", "image/bmp", "image/svg+xml", "image/avif"},
         {"Graphics", "Viewer"}},
        {"video",
         {"video/mp4", "video/x-matroska", "video/webm", "video/quicktime", "video/x-msvideo", "video/mpeg"},
         {"Video", "Player"}},
        {"music", {"audio/mpeg", "audio/flac", "audio/ogg", "audio/x-wav", "audio/mp4", "audio/x-opus+ogg"},
         {"Audio", "Player"}},
        {"pdf", {"application/pdf"}, {}},
    };
    return list;
}

const Category* findCategory(const QString& key) {
    const auto& list = categoryList();
    const auto it = std::ranges::find_if(list, [&](const Category& c) { return key == QLatin1String(c.key); });
    return it == list.end() ? nullptr : &*it;
}

} // namespace

DefaultApps* DefaultApps::create(QQmlEngine*, QJSEngine*) {
    static auto* d = new DefaultApps(QCoreApplication::instance());
    QJSEngine::setObjectOwnership(d, QJSEngine::CppOwnership);
    return d;
}

DefaultApps::DefaultApps(QObject* parent) : QObject(parent) {
    connect(Apps::instance(), &Apps::changed, this, &DefaultApps::changed);
    refresh();
}

bool DefaultApps::available() const {
    return !QStandardPaths::findExecutable(QStringLiteral("xdg-mime")).isEmpty();
}

void DefaultApps::refresh() {
    if (!available())
        return;
    for (const auto& c : categoryList())
        query(QLatin1String(c.key));
}

void DefaultApps::query(const QString& key) {
    const auto* category = findCategory(key);
    if (!category)
        return;
    auto* p = new QProcess(this);
    connect(p, &QProcess::finished, this, [this, p, key] {
        p->deleteLater();
        const auto id = QString::fromUtf8(p->readAllStandardOutput()).trimmed();
        if (m_current.value(key) != id) {
            m_current.insert(key, id);
            emit changed();
        }
    });
    p->start(QStringLiteral("xdg-mime"),
             {QStringLiteral("query"), QStringLiteral("default"), category->mimeTypes.first()});
}

void DefaultApps::setDefault(const QString& key, const QString& desktopId) {
    const auto* category = findCategory(key);
    if (!category || !available() || !Apps::instance()->entry(desktopId))
        return;
    auto* p = new QProcess(this);
    connect(p, &QProcess::finished, this, [this, p, key] {
        p->deleteLater();
        query(key);
    });
    p->start(QStringLiteral("xdg-mime"),
             QStringList{QStringLiteral("default"), desktopId} + category->mimeTypes);
}

QVariantList DefaultApps::categories() const {
    const auto& entries = Apps::instance()->entries();
    QVariantList out;
    for (const auto& c : categoryList()) {
        const QString key = QLatin1String(c.key);
        const auto current = m_current.value(key);
        QVariantList candidates;
        bool hasCurrent = false;
        for (const auto& e : entries) {
            const bool handles =
                std::ranges::any_of(c.mimeTypes, [&](const QString& m) { return e.mimeTypes.contains(m); }) ||
                (!c.menuCategories.isEmpty() &&
                 std::ranges::all_of(c.menuCategories, [&](const QString& m) { return e.categories.contains(m); }));
            if (!handles && e.id != current)
                continue;
            hasCurrent = hasCurrent || e.id == current;
            candidates.append(QVariantMap{
                {QStringLiteral("id"), e.id},
                {QStringLiteral("name"), e.name},
                {QStringLiteral("icon"), Icons::source(e.icon, QStringLiteral("application-x-executable"))},
            });
        }
        // Set to something not installed (or hidden): still show it.
        if (!current.isEmpty() && !hasCurrent) {
            candidates.prepend(QVariantMap{
                {QStringLiteral("id"), current},
                {QStringLiteral("name"), QString(current).remove(QLatin1String(".desktop"))},
                {QStringLiteral("icon"), Icons::source(QStringLiteral("application-x-executable"))},
            });
        }
        out.append(QVariantMap{
            {QStringLiteral("key"), key},
            {QStringLiteral("current"), current},
            {QStringLiteral("candidates"), candidates},
        });
    }
    return out;
}

} // namespace island
