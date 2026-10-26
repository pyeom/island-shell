#pragma once

#include <QDBusContext>
#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// org.islandshell.Control on the session bus. Method calls are turned into
// signals that the islands handle in QML.
class ShellControl : public QObject, protected QDBusContext {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // D-Bus properties (names fixed by the interface).
    Q_PROPERTY(bool Dnd READ dnd)
    Q_PROPERTY(QString Version READ version)
    // QML-facing.
    Q_PROPERTY(bool dnd READ dnd WRITE SetDnd NOTIFY dndChanged)

public:
    static constexpr auto kService = "org.islandshell.Shell";
    static constexpr auto kPath = "/org/islandshell/Shell";

    enum class Registration { Ok, NoBus, AlreadyRunning };

    static ShellControl* instance();
    static ShellControl* create(QQmlEngine*, QJSEngine*);

    Registration registerOnBus();

    bool dnd() const { return m_dnd; }
    QString version() const;

    // Opens the settings window on a page ("calendar"), with page-specific
    // arguments (calendar: { uid, start } of an event to show).
    Q_INVOKABLE void showSettingsPage(const QString& page, const QVariantMap& args = {}) {
        emit settingsPageRequested(page, args);
    }

public slots:
    // D-Bus methods.
    void ToggleLauncher();
    void Expand(const QString& activity);
    void Collapse();
    void Reload();
    void SetDnd(bool enabled);
    void ShowPower();
    void ShowQuickSettings();
    void Screenshot(const QString& mode);
    void Wallpaper(const QString& action);
    void ShowSettings();
    void ShowSettingsPage(const QString& page) { emit settingsPageRequested(page, {}); }
    void Media(const QString& action);
    void Volume(const QString& action);
    QStringList ListActivities();

signals:
    void launcherToggleRequested();
    void expandRequested(const QString& activity);
    void collapseRequested();
    void powerRequested();
    void quickSettingsRequested();
    void wallpaperPickRequested();
    void settingsRequested();
    void settingsPageRequested(const QString& page, const QVariantMap& args);
    void wallpaperRandomRequested();
    void reloadRequested();
    void dndChanged();

private:
    explicit ShellControl(QObject* parent = nullptr);
    bool m_dnd = false;
};

} // namespace island
