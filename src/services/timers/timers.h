#pragma once

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// Countdown timers started from the launcher ("15 min", "timer 5 pasta").
// A finished timer rings and stays until dismissed (or ten minutes pass).
class Timers : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // [{ id, label, duration (s), endsAt (date), done }], finished ones
    // first, then the soonest.
    Q_PROPERTY(QVariantList timers READ timers NOTIFY changed)

public:
    static Timers* instance();
    static Timers* create(QQmlEngine*, QJSEngine*);

    QVariantList timers() const;

    Q_INVOKABLE int start(int seconds, const QString& label = {});
    // Adds time to a running timer, or restarts a finished one with it.
    Q_INVOKABLE void extend(int id, int seconds);
    // Cancels a running timer or dismisses a finished one.
    Q_INVOKABLE void remove(int id);

signals:
    void changed();
    void finished(int id, const QString& label);

private:
    struct Entry {
        int id = 0;
        QString label;
        int duration = 0;
        QDateTime endsAt;
        bool done = false;
    };

    explicit Timers(QObject* parent = nullptr);
    void schedule();
    void check();
    void ring();

    QList<Entry> m_entries;
    QTimer m_next;
    int m_lastId = 0;
};

} // namespace island
