#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QSocketNotifier>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <memory>

class QQmlEngine;
class QJSEngine;

namespace island {

// One app playing audio (a PipeWire output stream), for the per-app mixer.
class AudioStream : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Listed by Audio.streams")

    Q_PROPERTY(int id READ id CONSTANT)
    Q_PROPERTY(QString name READ name NOTIFY changed)
    // What it plays ("YouTube - Firefox"), when the app says.
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(QString iconName READ iconName NOTIFY changed)
    Q_PROPERTY(qreal volume READ volume NOTIFY changed)
    Q_PROPERTY(bool muted READ muted NOTIFY changed)

public:
    explicit AudioStream(int id, QObject* parent) : QObject(parent), m_id(id) {}

    int id() const { return m_id; }
    QString name() const { return m_name; }
    QString title() const { return m_title; }
    QString iconName() const { return m_iconName; }
    qreal volume() const { return m_volume; }
    bool muted() const { return m_muted; }

    void setInfo(const QString& name, const QString& title, const QString& iconName);
    void setVolume(qreal volume, bool muted);

signals:
    void changed();

private:
    int m_id;
    QString m_name;
    QString m_title;
    QString m_iconName;
    qreal m_volume = 1;
    bool m_muted = false;
};

// Default output (sink) and input (microphone source) from PipeWire, and the
// apps playing audio.
//
// Runs PipeWire's loop on Qt's event loop (no extra thread). Follows the
// defaults chosen by the session manager ("default" metadata) and reads the
// nodes' Props. Writes go through wpctl so WirePlumber keeps device routes in
// sync, which writing node props directly would not.
class Audio : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    // Output. Perceived volume as pactl reports it: 1.0 is 100%, can exceed 1.
    Q_PROPERTY(qreal volume READ volume NOTIFY stateChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY stateChanged)
    Q_PROPERTY(QString sinkDescription READ sinkDescription NOTIFY stateChanged)
    // Microphone.
    Q_PROPERTY(bool hasMicrophone READ hasMicrophone NOTIFY stateChanged)
    Q_PROPERTY(qreal micVolume READ micVolume NOTIFY stateChanged)
    Q_PROPERTY(bool micMuted READ micMuted NOTIFY stateChanged)
    Q_PROPERTY(QString sourceDescription READ sourceDescription NOTIFY stateChanged)
    // [{ id, name, description, isDefault }]
    Q_PROPERTY(QVariantList sinks READ sinks NOTIFY devicesChanged)
    Q_PROPERTY(QVariantList sources READ sources NOTIFY devicesChanged)
    // Apps playing audio, by name.
    Q_PROPERTY(QList<island::AudioStream*> streams READ streams NOTIFY streamsChanged)

public:
    static Audio* instance();
    static Audio* create(QQmlEngine*, QJSEngine*);
    ~Audio() override;

    bool available() const;
    qreal volume() const;
    // The volume about to be set, else the current one: what keys that step
    // the volume start from while presses are still being sent.
    qreal targetVolume() const { return m_pendingVolume >= 0 ? m_pendingVolume : volume(); }
    bool muted() const;
    QString sinkDescription() const;
    bool hasMicrophone() const;
    qreal micVolume() const;
    bool micMuted() const;
    QString sourceDescription() const;
    QVariantList sinks() const;
    QVariantList sources() const;
    QList<AudioStream*> streams() const;

    // Coalesced while dragging a slider.
    Q_INVOKABLE void setVolume(qreal volume);
    Q_INVOKABLE void setMuted(bool muted);
    Q_INVOKABLE void setMicVolume(qreal volume);
    Q_INVOKABLE void setMicMuted(bool muted);
    // Makes a sink or source (by PipeWire id) the default.
    Q_INVOKABLE void setDefault(int id);
    // An app's volume (by stream id); coalesced like setVolume.
    Q_INVOKABLE void setStreamVolume(int id, qreal volume);
    Q_INVOKABLE void setStreamMuted(int id, bool muted);

    struct Private;

signals:
    void availableChanged();
    void stateChanged();
    void devicesChanged();
    void streamsChanged();
    // Output volume or mute changed (not emitted for the initial values).
    void changed();
    // Microphone mute changed (not emitted for the initial value).
    void micMuteChanged();

private:
    explicit Audio(QObject* parent = nullptr);
    void flushVolumes();

    std::unique_ptr<Private> d;
    QSocketNotifier* m_notifier = nullptr;
    QTimer m_volumeThrottle;
    qreal m_pendingVolume = -1;
    qreal m_pendingMicVolume = -1;
    QHash<int, qreal> m_pendingStreamVolumes;
};

} // namespace island
