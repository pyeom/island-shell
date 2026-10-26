#pragma once

#include "configdata.h"
#include "filewatch.h"

#include <QObject>
#include <QStringList>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// config.toml, reloaded when the file changes. Every property shares the
// `changed` signal: a reload replaces the whole configuration at once.
//
// Path: $ISLAND_SHELL_CONFIG, else $XDG_CONFIG_HOME/island-shell/config.toml.
class Config : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString path READ path CONSTANT)
    Q_PROPERTY(QStringList errors READ errors NOTIFY changed)
    // Bumped on every reload; bind to it to re-read Config.get(...) values.
    Q_PROPERTY(int revision READ revision NOTIFY changed)

    Q_PROPERTY(int compactWidth READ compactWidth NOTIFY changed)
    Q_PROPERTY(int compactHeight READ compactHeight NOTIFY changed)
    Q_PROPERTY(int peekExtraWidth READ peekExtraWidth NOTIFY changed)
    Q_PROPERTY(int peekExtraHeight READ peekExtraHeight NOTIFY changed)
    Q_PROPERTY(int expandedWidth READ expandedWidth NOTIFY changed)
    Q_PROPERTY(int expandedHeight READ expandedHeight NOTIFY changed)
    Q_PROPERTY(int expandedRadius READ expandedRadius NOTIFY changed)
    Q_PROPERTY(int topMargin READ topMargin NOTIFY changed)
    Q_PROPERTY(int exclusiveZone READ exclusiveZone NOTIFY changed)
    Q_PROPERTY(int surfaceWidth READ surfaceWidth NOTIFY changed)
    Q_PROPERTY(int surfaceHeight READ surfaceHeight NOTIFY changed)

    // "full" | "reduced" | "off"
    // [animations] mode, unless a power mode overrides it.
    Q_PROPERTY(QString animations READ animations NOTIFY animationsChanged)
    Q_PROPERTY(QString clockFormat READ clockFormat NOTIFY changed)
    Q_PROPERTY(int powerHoldMs READ powerHoldMs NOTIFY changed)
    // "minimal" | "hide" | "show"
    Q_PROPERTY(QString fullscreen READ fullscreen NOTIFY changed)

public:
    static Config* instance();
    static Config* create(QQmlEngine*, QJSEngine*);

    QString path() const { return m_watch.path(); }
    QStringList errors() const { return m_errors; }
    int revision() const { return m_revision; }
    const ConfigData& data() const { return m_data; }

    int compactWidth() const { return m_data.island.compactWidth; }
    int compactHeight() const { return m_data.island.compactHeight; }
    int peekExtraWidth() const { return m_data.island.peekExtraWidth; }
    int peekExtraHeight() const { return m_data.island.peekExtraHeight; }
    int expandedWidth() const { return m_data.island.expandedWidth; }
    int expandedHeight() const { return m_data.island.expandedHeight; }
    int expandedRadius() const { return m_data.island.expandedRadius; }
    int topMargin() const { return m_data.island.topMargin; }
    int exclusiveZone() const { return m_data.island.exclusiveZone; }
    int surfaceWidth() const { return m_data.island.surfaceWidth; }
    int surfaceHeight() const { return m_data.island.surfaceHeight; }
    QString animations() const;
    // "full", "reduced" or "off" while a power mode wants it; empty: the
    // configured mode.
    void setAnimationsOverride(const QString& mode);
    QString clockFormat() const { return QString::fromStdString(m_data.clock.format); }
    int powerHoldMs() const { return m_data.power.holdMs; }
    QString fullscreen() const { return QString::fromStdString(m_data.island.fullscreen); }

    // Any value by dotted path, for activity settings:
    //   Config.get("activities.weather.city", "Santiago")
    Q_INVOKABLE QVariant get(const QString& path, const QVariant& fallback = {}) const;

    // Writes one value ("theme.style", "blur") into config.toml, editing only
    // that line (comments and formatting are kept), then reloads.
    Q_INVOKABLE bool set(const QString& path, const QVariant& value);

public slots:
    void reload();

signals:
    void changed();
    void animationsChanged();

private:
    explicit Config(QObject* parent = nullptr);

    FileWatch m_watch;
    ConfigData m_data;
    QString m_animationsOverride;
    QStringList m_errors;
    int m_revision = 0;
};

} // namespace island
