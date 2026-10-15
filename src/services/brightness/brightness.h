#pragma once

#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// Screen backlight level (laptops), from /sys/class/backlight/<first device>.
// sysfs does not notify on writes, so the files are polled while a device
// exists. $ISLAND_SHELL_BACKLIGHT overrides the device directory (tests).
class Brightness : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(qreal level READ level NOTIFY changed) // 0..1

public:
    static Brightness* create(QQmlEngine*, QJSEngine*);

    bool available() const { return !m_dir.isEmpty(); }
    qreal level() const { return m_level; }

    // Through logind (no root needed); writes the file directly for a test
    // directory set with $ISLAND_SHELL_BACKLIGHT.
    Q_INVOKABLE void setLevel(qreal level);

signals:
    // Not emitted for the initial value.
    void changed();

private:
    explicit Brightness(QObject* parent = nullptr);
    void poll();

    QString m_dir;
    QTimer m_poll;
    qreal m_level = -1;
    bool m_fake = false;
};

} // namespace island
