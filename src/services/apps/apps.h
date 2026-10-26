#pragma once

#include "desktopentry.h"

#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <algorithm>

class QQmlEngine;
class QJSEngine;

namespace island {

// Installed applications (.desktop files in $XDG_DATA_HOME and
// $XDG_DATA_DIRS), searched and launched by the launcher. Usage counts live
// in $XDG_STATE_HOME/island-shell/launcher.json: one per window the app opens
// under Hyprland, however it was started; one per launch without Hyprland.
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
    // Runs a shell command in the configured terminal.
    Q_INVOKABLE void runInTerminal(const QString& command);
    // Whether `command` starts one of the installed graphical apps.
    bool isGraphical(const QString& command) const { return isGraphicalCommand(command, m_entries); }
    Q_INVOKABLE void openUrl(const QString& url);
    // Through wl-copy (survives the launcher closing), else QClipboard.
    Q_INVOKABLE void copyText(const QString& text);
    // Types text into the focused window with wtype, once the launcher has
    // given the keyboard back; does nothing without wtype.
    Q_INVOKABLE void typeText(const QString& text);
    // Icon for a window's app_id/class: StartupWMClass, then desktop file
    // name, then the class itself as an icon name.
    Q_INVOKABLE QString iconForClass(const QString& windowClass) const;
    const QList<DesktopEntry>& entries() const { return m_entries; }
    // By desktop file ID ("firefox.desktop"); nullptr if not installed.
    const DesktopEntry* entry(const QString& id) const {
        const auto it = std::ranges::find(m_entries, id, &DesktopEntry::id);
        return it == m_entries.end() ? nullptr : &*it;
    }

signals:
    void changed();

private:
    explicit Apps(QObject* parent = nullptr);
    void scan();
    void loadUsage();
    void saveUsage();
    void countWindow(const QString& windowClass);
    // StartupWMClass, then the desktop file name.
    const DesktopEntry* entryForClass(const QString& windowClass) const;
    QString terminal() const;

    QList<DesktopEntry> m_entries;
    QHash<QString, int> m_usage;
    QFileSystemWatcher m_watcher;
    QTimer m_rescan;
};

} // namespace island
