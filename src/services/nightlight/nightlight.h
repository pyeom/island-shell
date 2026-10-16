#pragma once

#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// Night light through wlsunset. Turning it off remembers the running
// process's command line (with its coordinates) so turning it on again
// starts the same one; [night_light] command overrides it.
class NightLight : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool running READ running NOTIFY changed)
    // wlsunset installed, or a command configured.
    Q_PROPERTY(bool available READ available CONSTANT)

public:
    static NightLight* create(QQmlEngine*, QJSEngine*);

    bool running() const { return m_pid > 0; }
    bool available() const;

    Q_INVOKABLE void setRunning(bool on);

signals:
    void changed();

private:
    explicit NightLight(QObject* parent = nullptr);
    void scan();
    QString command() const;
    QString statePath() const;

    qint64 m_pid = 0;
    QTimer m_poll;
};

} // namespace island
