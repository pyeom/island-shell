#pragma once

#include <QDBusConnection>
#include <QObject>
#include <QVariantMap>

namespace island {

// Mirror of one D-Bus object's properties on one interface: fetched with
// GetAll, kept current through PropertiesChanged.
class DBusProperties : public QObject {
    Q_OBJECT

public:
    DBusProperties(const QDBusConnection& bus, const QString& service, const QString& path,
                   const QString& interface, QObject* parent = nullptr);

    QString path() const { return m_path; }
    bool ready() const { return m_ready; }
    QVariant value(const QString& name) const { return m_values.value(name); }

    // Properties.Set, asynchronous. The new value arrives via `changed`.
    void set(const QString& name, const QVariant& value);
    void refresh();

signals:
    void changed();
    void failed(const QString& message);

private slots:
    void onPropertiesChanged(const QString& interface, const QVariantMap& changed,
                             const QStringList& invalidated);

private:
    QDBusConnection m_bus;
    QString m_service;
    QString m_path;
    QString m_interface;
    QVariantMap m_values;
    bool m_ready = false;
};

} // namespace island
