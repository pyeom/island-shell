#include "powerprofiles.h"

#include "dbusproperties.h"
#include "dbusutil.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QJSEngine>

namespace island {

PowerProfiles* PowerProfiles::create(QQmlEngine*, QJSEngine*) {
    static auto* pp = new PowerProfiles(QCoreApplication::instance());
    QJSEngine::setObjectOwnership(pp, QJSEngine::CppOwnership);
    return pp;
}

PowerProfiles::PowerProfiles(QObject* parent) : QObject(parent) {
    qDBusRegisterMetaType<QList<QVariantMap>>();
    const auto name = QStringLiteral("net.hadess.PowerProfiles");
    m_props = new DBusProperties(QDBusConnection::systemBus(), name,
                                 QStringLiteral("/net/hadess/PowerProfiles"), name, this);
    connect(m_props, &DBusProperties::changed, this, &PowerProfiles::changed);
}

bool PowerProfiles::available() const {
    return m_props->ready();
}

QString PowerProfiles::active() const {
    return m_props->value(QStringLiteral("ActiveProfile")).toString();
}

QStringList PowerProfiles::profiles() const {
    // Order them the way people think about them, not as the daemon lists them.
    static const QStringList order = {QStringLiteral("power-saver"), QStringLiteral("balanced"),
                                      QStringLiteral("performance")};
    QStringList available;
    for (const auto& p : dbus::cast<QList<QVariantMap>>(m_props->value(QStringLiteral("Profiles"))))
        available.append(p.value(QStringLiteral("Profile")).toString());
    QStringList out;
    for (const auto& p : order)
        if (available.contains(p))
            out.append(p);
    return out;
}

void PowerProfiles::setActive(const QString& profile) {
    m_props->set(QStringLiteral("ActiveProfile"), profile);
}

} // namespace island
