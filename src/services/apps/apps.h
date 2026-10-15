#pragma once

#include "desktopentry.h"

#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// Installed applications (.desktop files in $XDG_DATA_HOME and
// $XDG_DATA_DIRS), searched and launched by the launcher. Usage counts live
// in $XDG_STATE_HOME/island-shell/launcher.json.
class Apps : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int count READ count NOTIFY changed)

public:
    static Apps* instance();
    static Apps* create(QQmlEngine*, QJSEngine*);

    int count() const { return static_cast<int>(m_entries.size()); }

    // Best matches first: [{ id, name, genericName, comment, icon,
    // actions: [{ id, name, icon }] }]. Empty query: most used first.
    Q_INVOKABLE QVariantList search(const QString& query, int limit = 30) const;
    // Launches an app, or one of its desktop actions.
    Q_INVOKABLE bool launch(const QString& id, const QString& action = {});
    // Runs a shell command the same way apps are launched.
    Q_INVOKABLE void run(const QString& command);

signals:
    void changed();

private:
    explicit Apps(QObject* parent = nullptr);
    void scan();
    void loadUsage();
    void saveUsage();
    QString terminal() const;

    QList<DesktopEntry> m_entries;
    QHash<QString, int> m_usage;
    QFileSystemWatcher m_watcher;
    QTimer m_rescan;
};

} // namespace island
