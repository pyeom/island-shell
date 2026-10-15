#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// Session actions. Shutdown, reboot and suspend go through logind.
//
// Dry run (actions are logged, never run) when ISLAND_SHELL_DRY_RUN=1, and
// always under a nested Hyprland: logind is the host's, so "shut down" in a
// test compositor would power off the real machine.
class Power : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool dryRun READ dryRun CONSTANT)
    Q_PROPERTY(bool nested READ nested CONSTANT)

public:
    static Power* create(QQmlEngine*, QJSEngine*);

    bool dryRun() const { return m_dryRun; }
    bool nested() const { return m_nested; }

    // "shutdown" | "reboot" | "suspend" | "logout" | "lock"
    Q_INVOKABLE void run(const QString& action);

signals:
    // For tests and the UI: what ran (or would have, in dry-run).
    void ran(const QString& action);

private:
    explicit Power(QObject* parent = nullptr);
    void logind(const char* method);

    bool m_nested = false;
    bool m_dryRun = false;
};

} // namespace island
