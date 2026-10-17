#pragma once

#include "camerascan.h"

#include <QObject>
#include <QSocketNotifier>
#include <QStringList>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <memory>

class QQmlEngine;
class QJSEngine;

namespace island {

// What is using the microphone, camera and screen right now.
//
//   microphone: PipeWire capture streams (not sink monitors like cava, not
//               peak meters like pavucontrol)
//   camera:     processes holding /dev/video* open (inotify + /proc), since
//               many apps open V4L2 directly instead of through PipeWire
//   screen:     Hyprland "screencast" events (portal sharing)
class Privacy : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool microphone READ microphone NOTIFY changed)
    Q_PROPERTY(QStringList microphoneApps READ microphoneApps NOTIFY changed)
    Q_PROPERTY(bool camera READ camera NOTIFY changed)
    Q_PROPERTY(QStringList cameraApps READ cameraApps NOTIFY changed)
    Q_PROPERTY(bool screen READ screen NOTIFY changed)
    Q_PROPERTY(bool any READ any NOTIFY changed)

public:
    static Privacy* instance();
    static Privacy* create(QQmlEngine*, QJSEngine*);
    ~Privacy() override;

    bool microphone() const { return !m_micApps.isEmpty(); }
    QStringList microphoneApps() const { return m_micApps; }
    bool camera() const { return !m_cameraApps.isEmpty(); }
    QStringList cameraApps() const { return m_cameraApps; }
    bool screen() const { return m_screen; }
    bool any() const { return microphone() || camera() || screen(); }

    struct Private;
    void setMicrophoneApps(QStringList apps);

signals:
    void changed();

private:
    explicit Privacy(QObject* parent = nullptr);
    void setupPipeWire();
    void setupCamera();
    void scanCamera();

    std::unique_ptr<Private> d;
    QSocketNotifier* m_pwNotifier = nullptr;
    int m_inotify = -1;
    QSocketNotifier* m_inotifyNotifier = nullptr;
    QTimer m_cameraScan;
    CameraActivity m_cameraActivity;
    bool m_cameraScanned = false; // the first scan always runs
    QStringList m_micApps;
    QStringList m_cameraApps;
    bool m_screen = false;
};

} // namespace island
