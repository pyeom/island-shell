#pragma once

#include "calendarparse.h"

#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// Events from local calendars (vdir or .ics folders, see [calendar] paths).
// Syncing with Google, CalDAV, etc. is left to vdirsyncer.
class Calendar : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available NOTIFY changed)
    // Bumped on every reload, so QML bindings calling the methods re-run.
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    // The next event starting within [calendar] reminder_minutes, or {}.
    Q_PROPERTY(QVariantMap soon READ soon NOTIFY soonChanged)
    // Writable collections: [{ name, path, color }]; the default first.
    Q_PROPERTY(QVariantList collections READ collections NOTIFY changed)

public:
    static Calendar* create(QQmlEngine*, QJSEngine*);

    bool available() const { return !m_files.isEmpty(); }
    int revision() const { return m_revision; }
    QVariantMap soon() const { return m_soon; }
    QVariantList collections() const { return m_collections; }

    // "mañana 15:00 dentista" → { ok, summary, start, end, allDay, calendar }
    // or { ok: false, error }.
    Q_INVOKABLE QVariantMap previewQuick(const QString& text) const;
    Q_INVOKABLE bool addQuick(const QString& text);
    // Writes a new event (.ics) into `collectionPath` (empty: default).
    Q_INVOKABLE bool addEvent(const QString& summary, const QDateTime& start, const QDateTime& end,
                              bool allDay, const QString& collectionPath = {});

    // [{ summary, location, start, end, allDay, calendar, color }]
    Q_INVOKABLE QVariantList eventsOn(const QDate& day) const;
    // Events from now on (ongoing included), up to `limit`.
    Q_INVOKABLE QVariantList upcoming(int limit = 5) const;
    // Days of the month that have events.
    Q_INVOKABLE QList<int> busyDays(int year, int month) const;

    // Opens the new-event form (EventForm activity) on `day`.
    Q_INVOKABLE void requestNewEvent(const QDate& day) { emit newEventRequested(day); }

public slots:
    void reload();

signals:
    void changed();
    void newEventRequested(const QDate& day);
    void soonChanged();

private:
    explicit Calendar(QObject* parent = nullptr);
    void checkSoon();
    void scheduleMidnight();
    static QVariantMap toMap(const CalendarEvent& e);

    QString defaultCollection() const;

    QList<CalendarEvent> m_events;
    QVariantList m_collections;
    QStringList m_files;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    QTimer m_midnight;
    QTimer m_soonTimer;
    QVariantMap m_soon;
    int m_revision = 0;
};

} // namespace island
