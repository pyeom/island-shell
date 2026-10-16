#include "shellcontrol.h"

#include "activity.h"
#include "activityregistry.h"
#include "config.h"
#include "screenshots.h"
#include "controladaptor.h"
#include "theme.h"
#include "useractivities.h"

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

QStringList ShellControl::ListActivities() {
    QStringList names;
    for (const auto* a : ActivityRegistry::instance()->activities())
        if (!a->name().isEmpty())
            names.append(a->name());
    return names;
}

} // namespace island
