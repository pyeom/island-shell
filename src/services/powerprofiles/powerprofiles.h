#pragma once

#include <QObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

class DBusProperties;

// power-profiles-daemon: "power-saver" | "balanced" | "performance".
class PowerProfiles : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(QString active READ active NOTIFY changed)
    Q_PROPERTY(QStringList profiles READ profiles NOTIFY changed)

public:
    static PowerProfiles* create(QQmlEngine*, QJSEngine*);

    bool available() const;
    QString active() const;
    QStringList profiles() const;

    Q_INVOKABLE void setActive(const QString& profile);

signals:
    void changed();

private:
    explicit PowerProfiles(QObject* parent = nullptr);

    DBusProperties* m_props = nullptr;
};

} // namespace island
