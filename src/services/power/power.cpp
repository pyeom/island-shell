#include "power.h"

#include "config.h"
#include "hyprland.h"
#include "nesteddetect.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QJSEngine>
#include <QLoggingCategory>
#include <QProcess>

Q_LOGGING_CATEGORY(lcPower, "island.power")

namespace island {

Power* Power::create(QQmlEngine*, QJSEngine*) {
    static auto* power = new Power(QCoreApplication::instance());
    QJSEngine::setObjectOwnership(power, QJSEngine::CppOwnership);
    return power;
}

Power::Power(QObject* parent)
    : QObject(parent), m_nested(runningUnderNestedHyprland()),
      m_dryRun(m_nested || qEnvironmentVariableIntValue("ISLAND_SHELL_DRY_RUN") != 0) {
    if (m_nested)
        qCInfo(lcPower) << "nested Hyprland: power actions are dry runs";
}

void Power::logind(const char* method) {
    auto msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.login1"), QStringLiteral("/org/freedesktop/login1"),
        QStringLiteral("org.freedesktop.login1.Manager"), QLatin1String(method));
    msg.setArguments({true}); // interactive: allow polkit to ask
    QDBusConnection::systemBus().asyncCall(msg);
}

void Power::run(const QString& action) {
    qCInfo(lcPower).noquote() << (m_dryRun ? "dry run:" : "run:") << action;
    emit ran(action);
    if (m_dryRun)
        return;
    if (action == u"shutdown")
        logind("PowerOff");
    else if (action == u"reboot")
        logind("Reboot");
    else if (action == u"suspend")
        logind("Suspend");
    else if (action == u"logout")
        Hyprland::instance()->exit();
    else if (action == u"lock")
        QProcess::startDetached(
            QStringLiteral("sh"),
            {QStringLiteral("-c"),
             QString::fromStdString(Config::instance()->data().power.lockCommand)});
    else
        qCWarning(lcPower).noquote() << "unknown action" << action;
}

} // namespace island
