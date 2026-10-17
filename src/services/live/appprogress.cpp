#include "appprogress.h"

#include "apps.h"
#include "config.h"
#include "liveactivities.h"
#include "progressparse.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>

using namespace Qt::StringLiterals;

namespace island {

AppProgress* AppProgress::instance() {
    static auto* progress = new AppProgress(QCoreApplication::instance());
    return progress;
}

AppProgress::AppProgress(QObject* parent) : QObject(parent) {
    // Any path: each app picks its own.
    QDBusConnection::sessionBus().connect({}, {}, u"com.canonical.Unity.LauncherEntry"_s, u"Update"_s, this,
                                          SLOT(onUpdate(QString, QVariantMap)));
}

void AppProgress::onUpdate(const QString& appUri, const QVariantMap& props) {
    if (!Config::instance()->data().live.appProgress)
        return;
    const auto u = parseLauncherEntry(appUri, props);
    if (u.desktopId.isEmpty())
        return;
    auto* live = LiveActivities::instance();
    const auto id = u"progress:"_s + u.desktopId;

    if (u.progressVisible == false) {
        if (!live->contains(id))
            return;
        // Hidden at the end: done. Hidden halfway: cancelled.
        if (m_progress.value(id) >= 0.99) {
            live->set(id, {{u"state"_s, u"success"_s},
                           {u"progress"_s, 1.0},
                           {u"subtitle"_s, QCoreApplication::translate("AppProgress", "Done")},
                           {u"ttl"_s, 6}});
            live->disown(id); // apps often quit right after
        } else
            live->End(id);
        m_progress.remove(id);
        return;
    }
    // Counts (unread messages) without progress are not activities.
    if (!u.progress || (u.progressVisible != true && !live->contains(id)))
        return;

    m_progress.insert(id, *u.progress);
    QVariantMap update{{u"progress"_s, *u.progress}, {u"ttl"_s, 300}}; // gone quiet: drop it
    if (!live->contains(id)) {
        const auto* entry = Apps::instance()->entry(u.desktopId);
        update.insert(u"title"_s, entry ? entry->name : u.desktopId.chopped(u.desktopId.endsWith(u".desktop"_s) ? 8 : 0));
        update.insert(u"icon"_s, entry ? entry->icon : QString());
        update.insert(u"subtitle"_s, QCoreApplication::translate("AppProgress", "In progress"));
    }
    live->set(id, update, message().service());
}

} // namespace island
