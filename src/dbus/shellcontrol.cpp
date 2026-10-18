#include "shellcontrol.h"

#include "activity.h"
#include "activityregistry.h"
#include "audio.h"
#include "config.h"
#include "screenshots.h"
#include "controladaptor.h"
#include "mpris.h"
#include "theme.h"
#include "useractivities.h"
#include "volumestep.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QJSEngine>

namespace island {

ShellControl* ShellControl::instance() {
    static auto* control = new ShellControl(QCoreApplication::instance());
    return control;
}

ShellControl* ShellControl::create(QQmlEngine*, QJSEngine*) {
    auto* control = instance();
    QJSEngine::setObjectOwnership(control, QJSEngine::CppOwnership);
    return control;
}

ShellControl::ShellControl(QObject* parent) : QObject(parent) {
    new ControlAdaptor(this);
}

ShellControl::Registration ShellControl::registerOnBus() {
    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return Registration::NoBus;
    if (!bus.registerService(QLatin1String(kService)))
        return Registration::AlreadyRunning;
    bus.registerObject(QLatin1String(kPath), this);
    return Registration::Ok;
}

QString ShellControl::version() const {
    return QCoreApplication::applicationVersion();
}

void ShellControl::ToggleLauncher() {
    emit launcherToggleRequested();
}

void ShellControl::Expand(const QString& activity) {
    if (!activity.isEmpty() && !ActivityRegistry::instance()->find(activity)) {
        if (calledFromDBus())
            sendErrorReply(QStringLiteral("org.islandshell.Error.UnknownActivity"),
                           QStringLiteral("No activity named \"%1\"").arg(activity));
        return;
    }
    emit expandRequested(activity);
}

void ShellControl::Collapse() {
    emit collapseRequested();
}

void ShellControl::Reload() {
    Config::instance()->reload();
    Theme::instance()->reload();
    UserActivities::instance()->reload();
    emit reloadRequested();
}

void ShellControl::SetDnd(bool enabled) {
    if (m_dnd == enabled)
        return;
    m_dnd = enabled;
    emit dndChanged();
}

void ShellControl::ShowPower() {
    emit powerRequested();
}

void ShellControl::ShowQuickSettings() {
    emit quickSettingsRequested();
}

void ShellControl::Screenshot(const QString& mode) {
    if (!Screenshots::instance()->take(mode) && calledFromDBus())
        sendErrorReply(QStringLiteral("org.islandshell.Error.InvalidArgs"),
                       QStringLiteral("mode must be region, window, output or active "
                                      "(or a screenshot is already in progress)"));
}

void ShellControl::Wallpaper(const QString& action) {
    if (action == u"pick")
        emit wallpaperPickRequested();
    else if (action == u"random")
        emit wallpaperRandomRequested();
    else if (calledFromDBus())
        sendErrorReply(QStringLiteral("org.islandshell.Error.InvalidArgs"),
                       QStringLiteral("action must be pick or random"));
}

void ShellControl::ShowSettings() {
    emit settingsRequested();
}

void ShellControl::Media(const QString& action) {
    // The player the island shows, so keys and the pill agree.
    auto* player = Mpris::instance()->current();
    const bool known = action == u"play-pause" || action == u"next" || action == u"previous";
    if (!known || !player) {
        if (calledFromDBus())
            sendErrorReply(known ? QStringLiteral("org.islandshell.Error.NoPlayer")
                                 : QStringLiteral("org.islandshell.Error.InvalidArgs"),
                           known ? QStringLiteral("Nothing is playing")
                                 : QStringLiteral("action must be play-pause, next or previous"));
        return;
    }
    if (action == u"play-pause")
        player->playPause();
    else if (action == u"next")
        player->next();
    else
        player->previous();
}

void ShellControl::Volume(const QString& action) {
    auto* audio = Audio::instance();
    if (action == u"mute") {
        audio->setMuted(!audio->muted());
        return;
    }
    if (action == u"mic-mute") {
        audio->setMicMuted(!audio->micMuted());
        return;
    }
    const auto target = volumeTarget(action, audio->targetVolume());
    if (!target) {
        if (calledFromDBus())
            sendErrorReply(QStringLiteral("org.islandshell.Error.InvalidArgs"),
                           QStringLiteral("action must be up, down, mute, mic-mute, +N, -N or N"));
        return;
    }
    // Changing the volume unmutes, as volume keys usually do.
    if (audio->muted())
        audio->setMuted(false);
    audio->setVolume(*target);
}

QStringList ShellControl::ListActivities() {
    QStringList names;
    for (const auto* a : ActivityRegistry::instance()->activities())
        if (!a->name().isEmpty())
            names.append(a->name());
    return names;
}

} // namespace island
