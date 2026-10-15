#pragma once

#include <QObject>
#include <QSocketNotifier>
#include <QtQml/qqmlregistration.h>

#include <memory>

class QQmlEngine;
class QJSEngine;

namespace island {

// Default audio sink volume and mute, from PipeWire.
//
// Runs PipeWire's loop on Qt's event loop (no extra thread). Follows the
// default sink chosen by the session manager ("default" metadata) and reads
// the node's Props. `changed` only fires for changes after startup, which is
// what the volume OSD needs.
class Audio : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    // Perceived volume as pactl reports it: 1.0 is 100%, can exceed 1.
    Q_PROPERTY(qreal volume READ volume NOTIFY stateChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY stateChanged)
    Q_PROPERTY(QString sinkDescription READ sinkDescription NOTIFY stateChanged)

public:
    static Audio* instance();
    static Audio* create(QQmlEngine*, QJSEngine*);
    ~Audio() override;

    bool available() const;
    qreal volume() const;
    bool muted() const;
    QString sinkDescription() const;

    struct Private;

signals:
    void availableChanged();
    void stateChanged();
    // Volume or mute changed (not emitted for the initial values).
    void changed();

private:
    explicit Audio(QObject* parent = nullptr);

    std::unique_ptr<Private> d;
    QSocketNotifier* m_notifier = nullptr;
};

} // namespace island
