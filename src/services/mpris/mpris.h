#pragma once

#include <QDateTime>
#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// One org.mpris.MediaPlayer2.* service.
class MprisPlayer : public QObject {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(QString service READ service CONSTANT)
    Q_PROPERTY(QString identity READ identity NOTIFY changed)
    Q_PROPERTY(QString desktopEntry READ desktopEntry NOTIFY changed)
    // "Playing" | "Paused" | "Stopped"
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(bool playing READ playing NOTIFY changed)
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(QString artist READ artist NOTIFY changed)
    Q_PROPERTY(QString album READ album NOTIFY changed)
    Q_PROPERTY(QString artUrl READ artUrl NOTIFY changed)
    // Seconds; length 0 when unknown (streams).
    Q_PROPERTY(qreal length READ length NOTIFY changed)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged)
    Q_PROPERTY(bool canGoNext READ canGoNext NOTIFY changed)
    Q_PROPERTY(bool canGoPrevious READ canGoPrevious NOTIFY changed)
    Q_PROPERTY(bool canSeek READ canSeek NOTIFY changed)
    Q_PROPERTY(bool canControl READ canControl NOTIFY changed)

public:
    explicit MprisPlayer(const QString& service, QObject* parent = nullptr);

    QString service() const { return m_service; }
    QString identity() const { return m_identity; }
    QString desktopEntry() const { return m_desktopEntry; }
    QString status() const { return m_status; }
    bool playing() const { return m_status == u"Playing"; }
    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    QString album() const { return m_album; }
    QString artUrl() const { return m_artUrl; }
    qreal length() const { return static_cast<qreal>(m_lengthUs) / 1e6; }
    qreal position() const;
    bool canGoNext() const { return m_canGoNext; }
    bool canGoPrevious() const { return m_canGoPrevious; }
    bool canSeek() const { return m_canSeek; }
    bool canControl() const { return m_canControl; }

    Q_INVOKABLE void playPause();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void seekTo(qreal seconds);
    Q_INVOKABLE void raise();

signals:
    void changed();
    void positionChanged();
    void startedPlaying();

private slots:
    void onPropertiesChanged(const QString& interface, const QVariantMap& changed,
                             const QStringList& invalidated);
    void onSeeked(qlonglong positionUs);

private:
    void call(const QString& method, const QVariantList& args = {});
    void fetchAll();
    void fetchPosition();
    void applyPlayer(const QVariantMap& props);
    void applyMetadata(const QVariantMap& metadata);

    QString m_service;
    QString m_identity;
    QString m_desktopEntry;
    QString m_status = QStringLiteral("Stopped");
    QString m_title;
    QString m_artist;
    QString m_album;
    QString m_artUrl;
    QString m_trackId;
    qlonglong m_lengthUs = 0;
    qlonglong m_positionUs = 0;
    double m_rate = 1.0;
    QElapsedTimer m_positionClock;
    QTimer m_positionTick;
    bool m_canGoNext = false;
    bool m_canGoPrevious = false;
    bool m_canSeek = false;
    bool m_canControl = false;
};

// All MPRIS players on the session bus. `current` is the one most recently
// playing (kept after pausing until another starts).
class Mpris : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QList<island::MprisPlayer*> players READ players NOTIFY playersChanged)
    Q_PROPERTY(island::MprisPlayer* current READ current NOTIFY currentChanged)

public:
    static Mpris* instance();
    static Mpris* create(QQmlEngine*, QJSEngine*);

    QList<MprisPlayer*> players() const;
    MprisPlayer* current() const { return m_current; }
    Q_INVOKABLE void setCurrent(island::MprisPlayer* player);

signals:
    void playersChanged();
    void currentChanged();

private slots:
    void onNameOwnerChanged(const QString& name, const QString& oldOwner, const QString& newOwner);

private:
    explicit Mpris(QObject* parent = nullptr);
    void add(const QString& service);
    void remove(const QString& service);
    void pickCurrent();

    QList<QPointer<MprisPlayer>> m_players;
    QPointer<MprisPlayer> m_current;
};

} // namespace island
