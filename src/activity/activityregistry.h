#pragma once

#include <QList>
#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

class Activity;

// Every live Activity, in registration order. `changed` is coalesced: many
// property updates in one event-loop turn produce one signal.
class ActivityRegistry : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QList<Activity*> activities READ activities NOTIFY changed)

public:
    static ActivityRegistry* instance();
    static ActivityRegistry* create(QQmlEngine*, QJSEngine*);

    QList<Activity*> activities() const;
    Q_INVOKABLE island::Activity* find(const QString& name) const;

    void add(Activity* activity);
    void remove(Activity* activity);

signals:
    void changed();
    void alertRequested(island::Activity* activity, int durationMs);

private:
    explicit ActivityRegistry(QObject* parent = nullptr);
    void scheduleChanged();

    QList<QPointer<Activity>> m_activities;
    bool m_pending = false;
};

} // namespace island
