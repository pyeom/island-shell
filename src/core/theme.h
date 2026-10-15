#pragma once

#include "filewatch.h"
#include "pywal.h"

#include <QColor>
#include <QObject>
#include <QtQml/qqmlregistration.h>

#include <optional>

class QQmlEngine;
class QJSEngine;

namespace island {

// Island colors from config.toml [theme], optionally sourced from pywal
// (~/.cache/wal/colors.json, reloaded when `wal` rewrites it).
class Theme : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QColor background READ background NOTIFY changed)
    Q_PROPERTY(QColor foreground READ foreground NOTIFY changed)
    Q_PROPERTY(QColor secondaryForeground READ secondaryForeground NOTIFY changed)
    Q_PROPERTY(QColor accent READ accent NOTIFY changed)
    Q_PROPERTY(QColor danger READ danger CONSTANT)
    Q_PROPERTY(bool blur READ blur NOTIFY changed)

public:
    static Theme* instance();
    static Theme* create(QQmlEngine*, QJSEngine*);

    QColor background() const { return m_background; }
    QColor foreground() const { return m_foreground; }
    QColor secondaryForeground() const { return m_secondary; }
    QColor accent() const { return m_accent; }
    QColor danger() const { return {0xff, 0x45, 0x3a}; }
    bool blur() const { return m_blur; }

public slots:
    void reload();

signals:
    void changed();

private:
    explicit Theme(QObject* parent = nullptr);
    void apply();

    FileWatch m_pywalWatch;
    std::optional<PywalColors> m_pywal;
    QColor m_background;
    QColor m_foreground;
    QColor m_secondary;
    QColor m_accent;
    bool m_blur = false;
};

} // namespace island
