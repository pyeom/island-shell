#include "activity.h"

#include "activityregistry.h"

namespace island {

Activity::Activity(QObject* parent) : QObject(parent) {}

Activity::~Activity() {
    ActivityRegistry::instance()->remove(this);
}

void Activity::setName(const QString& name) {
    if (m_name == name)
        return;
    m_name = name;
    emit nameChanged();
}

void Activity::setPriority(int priority) {
    if (m_priority == priority)
        return;
    m_priority = priority;
    emit priorityChanged();
}

void Activity::setActive(bool active) {
    if (m_active == active)
        return;
    m_active = active;
    emit activeChanged();
}

void Activity::setScope(Scope scope) {
    if (m_scope == scope)
        return;
    m_scope = scope;
    emit scopeChanged();
}

void Activity::setModal(bool modal) {
    if (m_modal == modal)
        return;
    m_modal = modal;
    emit modalChanged();
}

void Activity::setComponent(QPointer<QQmlComponent>& slot, QQmlComponent* c) {
    if (slot == c)
        return;
    slot = c;
    emit presentationsChanged();
}

void Activity::setCompactLeading(QQmlComponent* c) { setComponent(m_compactLeading, c); }
void Activity::setCompactTrailing(QQmlComponent* c) { setComponent(m_compactTrailing, c); }
void Activity::setMinimal(QQmlComponent* c) { setComponent(m_minimal, c); }
void Activity::setExpanded(QQmlComponent* c) { setComponent(m_expanded, c); }

QQmlListProperty<QObject> Activity::data() {
    return {this, &m_data,
            [](QQmlListProperty<QObject>* list, QObject* obj) {
                obj->setParent(list->object);
                static_cast<QList<QObject*>*>(list->data)->append(obj);
            },
            [](QQmlListProperty<QObject>* list) {
                return static_cast<QList<QObject*>*>(list->data)->size();
            },
            [](QQmlListProperty<QObject>* list, qsizetype i) {
                return static_cast<QList<QObject*>*>(list->data)->at(i);
            },
            [](QQmlListProperty<QObject>* list) {
                static_cast<QList<QObject*>*>(list->data)->clear();
            }};
}

void Activity::alert(int durationMs) {
    emit alertRequested(durationMs);
}

void Activity::dismiss() {
    emit dismissRequested();
}

void Activity::componentComplete() {
    if (m_name.isEmpty())
        qWarning("island-shell: Activity without a name; it cannot be targeted by islandctl");
    ActivityRegistry::instance()->add(this);
}

} // namespace island
