#include "activityregistry.h"

#include "activity.h"

#include <QCoreApplication>
#include <QJSEngine>

namespace island {

ActivityRegistry* ActivityRegistry::instance() {
    static auto* registry = new ActivityRegistry(QCoreApplication::instance());
    return registry;
}

ActivityRegistry* ActivityRegistry::create(QQmlEngine*, QJSEngine*) {
    auto* registry = instance();
    QJSEngine::setObjectOwnership(registry, QJSEngine::CppOwnership);
    return registry;
}

ActivityRegistry::ActivityRegistry(QObject* parent) : QObject(parent) {}

QList<Activity*> ActivityRegistry::activities() const {
    QList<Activity*> out;
    for (const auto& a : m_activities)
        if (a)
            out.append(a);
    return out;
}

Activity* ActivityRegistry::find(const QString& name) const {
    for (const auto& a : m_activities)
        if (a && a->name() == name)
            return a;
    return nullptr;
}

void ActivityRegistry::add(Activity* activity) {
    if (m_activities.contains(activity))
        return;
    if (!activity->name().isEmpty() && find(activity->name()))
        qWarning("island-shell: duplicate activity name \"%s\"", qPrintable(activity->name()));
    m_activities.append(activity);

    for (auto sig : {&Activity::activeChanged, &Activity::priorityChanged, &Activity::scopeChanged,
                     &Activity::presentationsChanged, &Activity::nameChanged})
        connect(activity, sig, this, &ActivityRegistry::scheduleChanged);
    connect(activity, &Activity::alertRequested, this,
            [this, activity](int ms) { emit alertRequested(activity, ms); });
    scheduleChanged();
}

void ActivityRegistry::remove(Activity* activity) {
    if (m_activities.removeAll(activity) > 0) {
        disconnect(activity, nullptr, this, nullptr);
        scheduleChanged();
    }
}

void ActivityRegistry::scheduleChanged() {
    if (m_pending)
        return;
    m_pending = true;
    QMetaObject::invokeMethod(
        this,
        [this] {
            m_pending = false;
            emit changed();
        },
        Qt::QueuedConnection);
}

} // namespace island
