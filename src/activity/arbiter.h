#pragma once

#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

namespace island {

class Activity;

// Per-island view of the registry: which activity owns the pill and which
// one sits in the secondary bubble. See selectActivities() for the rules.
class Arbiter : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool focused READ focused WRITE setFocused NOTIFY focusedChanged)
    Q_PROPERTY(island::Activity* primary READ primary NOTIFY selectionChanged)
    Q_PROPERTY(island::Activity* secondary READ secondary NOTIFY selectionChanged)

public:
    explicit Arbiter(QObject* parent = nullptr);

    bool focused() const { return m_focused; }
    void setFocused(bool focused);

    Activity* primary() const { return m_primary; }
    Activity* secondary() const { return m_secondary; }

    // Whether `activity` may appear on this island at all.
    Q_INVOKABLE bool eligible(island::Activity* activity) const;

signals:
    void focusedChanged();
    void selectionChanged();

private:
    void update();
    // The activity's priority, or its [priorities] override.
    static int priority(const Activity* activity);

    bool m_focused = false;
    QPointer<Activity> m_primary;
    QPointer<Activity> m_secondary;
};

} // namespace island
