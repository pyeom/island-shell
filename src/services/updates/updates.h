#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// Pending Arch updates: checkupdates (pacman-contrib) for the repos, plus
// paru/yay -Qua for the AUR. Checks every [updates] interval and again when
// the local package database changes (an upgrade, from here or elsewhere).
class Updates : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // checkupdates installed.
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(bool checking READ checking NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(int aurCount READ aurCount NOTIFY changed)
    // "name old -> new", repos first.
    Q_PROPERTY(QStringList packages READ packages NOTIFY changed)
    // The last check failed (offline, mirror down...).
    Q_PROPERTY(bool failed READ failed NOTIFY changed)
    // A check has finished since startup.
    Q_PROPERTY(bool checked READ checked NOTIFY changed)

public:
    static Updates* instance();
    static Updates* create(QQmlEngine*, QJSEngine*);

    bool available() const;
    bool checking() const { return m_step != Step::Idle; }
    int count() const { return static_cast<int>(m_packages.size()); }
    int aurCount() const { return m_aurCount; }
    QStringList packages() const { return m_packages; }
    bool failed() const { return m_failed; }
    bool checked() const { return m_checked; }

    Q_INVOKABLE void check();
    // Runs the upgrade command in the terminal (through island-run when
    // installed, so the island shows how it goes).
    Q_INVOKABLE void upgrade();

signals:
    void changed();

private:
    enum class Step { Idle, Repo, Aur };

    explicit Updates(QObject* parent = nullptr);
    void applyInterval();
    void runStep(Step step);
    QString aurHelper() const;

    Step m_step = Step::Idle;
    QStringList m_packages;
    QStringList m_found; // during a check
    int m_aurCount = 0;
    bool m_failed = false;
    bool m_checked = false;
    bool m_recheck = false;
    QTimer m_timer;
    QTimer m_dbChanged;
    QFileSystemWatcher m_watcher;
};

} // namespace island
