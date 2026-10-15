#pragma once

#include <QFileSystemWatcher>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;
class QQmlError;

namespace island {

// Loads every *.qml file in $XDG_CONFIG_HOME/island-shell/activities/ as an
// activity and reloads them when files change. Helper components go in
// subdirectories and are imported from the activity files.
//
// Each file is created in isolation: one that fails to compile or throws is
// reported in `errors` and skipped, the rest keep running.
class UserActivities : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString directory READ directory CONSTANT)
    Q_PROPERTY(QStringList loaded READ loaded NOTIFY changed)
    // [{ file, message }]
    Q_PROPERTY(QVariantList errors READ errors NOTIFY changed)

public:
    static UserActivities* instance();
    static UserActivities* create(QQmlEngine*, QJSEngine*);

    void setEngine(QQmlEngine* engine);

    QString directory() const { return m_directory; }
    QStringList loaded() const;
    QVariantList errors() const { return m_errors; }

public slots:
    void reload();

signals:
    void changed();

private:
    explicit UserActivities(QObject* parent = nullptr);
    void watch();
    void addError(const QString& file, const QString& message);
    void onEngineWarnings(const QList<QQmlError>& warnings);

    QString m_directory;
    QPointer<QQmlEngine> m_engine;
    QList<QPointer<QObject>> m_objects;
    QStringList m_files;
    QVariantList m_errors;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
};

} // namespace island
