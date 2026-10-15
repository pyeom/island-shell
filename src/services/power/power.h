#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// Session actions. Shutdown, reboot and suspend go through logind.
//
// ISLAND_SHELL_DRY_RUN=1 logs actions instead of running them.
class Power : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool dryRun READ dryRun CONSTANT)

public:
    static Power* create(QQmlEngine*, QJSEngine*);

    bool dryRun() const { return m_dryRun; }

    // "shutdown" | "reboot" | "suspend" | "logout" | "lock"
    Q_INVOKABLE void run(const QString& action);

signals:
    // For tests and the UI: what ran (or would have, in dry-run).
    void ran(const QString& action);

private:
    explicit Power(QObject* parent = nullptr);
    void logind(const char* method);

    bool m_dryRun = false;
};

} // namespace island
