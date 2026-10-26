#include "dbusproperties.h"

#include "dbusutil.h"

#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>

namespace island {

namespace {
constexpr auto kProps = "org.freedesktop.DBus.Properties";
}

DBusProperties::DBusProperties(const QDBusConnection& bus, const QString& service,
                               const QString& path, const QString& interface, QObject* parent)
    : QObject(parent), m_bus(bus), m_service(service), m_path(path), m_interface(interface) {
    m_bus.connect(m_service, m_path, QLatin1String(kProps), QStringLiteral("PropertiesChanged"),
                  this, SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));
    auto* watcher = new QDBusServiceWatcher(m_service, m_bus, QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, this,
            [this](const QString&, const QString&, const QString& newOwner) {
                if (!newOwner.isEmpty()) {
                    refresh();
                    return;
                }
                m_values.clear();
                m_ready = false;
                emit changed();
            });
    refresh();
}

void DBusProperties::refresh() {
    auto msg = QDBusMessage::createMethodCall(m_service, m_path, QLatin1String(kProps),
                                              QStringLiteral("GetAll"));
    msg.setArguments({m_interface});
    auto* watcher = new QDBusPendingCallWatcher(m_bus.asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
        QDBusPendingReply<QVariantMap> reply = *w;
        w->deleteLater();
        if (reply.isError()) {
            emit failed(reply.error().message());
            return;
        }
        m_values = reply.value();
        m_ready = true;
        emit changed();
    });
}

void DBusProperties::set(const QString& name, const QVariant& value) {
    auto msg = QDBusMessage::createMethodCall(m_service, m_path, QLatin1String(kProps),
                                              QStringLiteral("Set"));
    msg.setArguments({m_interface, name, QVariant::fromValue(QDBusVariant(value))});
    auto* watcher = new QDBusPendingCallWatcher(m_bus.asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
        w->deleteLater();
        if (w->isError())
            emit failed(w->error().message());
    });
}

void DBusProperties::onPropertiesChanged(const QString& interface, const QVariantMap& changedProps,
                                         const QStringList& invalidated) {
    if (interface != m_interface)
        return;
    for (auto it = changedProps.cbegin(); it != changedProps.cend(); ++it)
        m_values.insert(it.key(), dbus::unwrap(it.value()));
    if (!invalidated.isEmpty())
        refresh();
    emit changed();
}

} // namespace island
