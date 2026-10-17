#pragma once

#include "sysparse.h"

#include <QObject>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// CPU, GPU (amdgpu sysfs), RAM and temperatures (hwmon), sampled every 2 s
// while something shows them (addViewer), else only the temperatures, every
// 10 s, for the heat alert. Missing sensors read as -1. `hot` turns on when a temperature stays at or
// above its [system] threshold for 10 s, and off 5 °C below it.
class SystemStats : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(qreal cpuUsage READ cpuUsage NOTIFY changed)       // 0..1
    Q_PROPERTY(qreal cpuTemp READ cpuTemp NOTIFY changed)         // °C
    Q_PROPERTY(qreal gpuUsage READ gpuUsage NOTIFY changed)       // 0..1
    Q_PROPERTY(qreal gpuTemp READ gpuTemp NOTIFY changed)
    Q_PROPERTY(qreal vramUsage READ vramUsage NOTIFY changed)     // 0..1
    Q_PROPERTY(qreal memoryUsage READ memoryUsage NOTIFY changed) // 0..1
    Q_PROPERTY(qreal nvmeTemp READ nvmeTemp NOTIFY changed)
    Q_PROPERTY(bool hot READ hot NOTIFY hotChanged)
    // "cpu" | "gpu" | "nvme" while hot
    Q_PROPERTY(QString hotSensor READ hotSensor NOTIFY hotChanged)
    Q_PROPERTY(qreal hotTemp READ hotTemp NOTIFY changed)

public:
    static SystemStats* create(QQmlEngine*, QJSEngine*);

    qreal cpuUsage() const { return m_cpuUsage; }
    qreal cpuTemp() const { return m_cpuTemp; }
    qreal gpuUsage() const { return m_gpuUsage; }
    qreal gpuTemp() const { return m_gpuTemp; }
    qreal vramUsage() const { return m_vramUsage; }
    qreal memoryUsage() const { return m_memoryUsage; }
    qreal nvmeTemp() const { return m_nvmeTemp; }
    bool hot() const { return !m_hotSensor.isEmpty(); }
    QString hotSensor() const { return m_hotSensor; }
    qreal hotTemp() const;

    // Gauges call these while they are on screen, for 2 s updates.
    Q_INVOKABLE void addViewer();
    Q_INVOKABLE void removeViewer();

signals:
    void changed();
    void hotChanged();

private:
    explicit SystemStats(QObject* parent = nullptr);
    void findSensors();
    void sample();
    void checkHeat();
    void updateInterval();

    QString m_cpuTempPath;
    QString m_gpuTempPath;
    QString m_nvmeTempPath;
    QString m_gpuDir;
    sys::CpuTimes m_lastCpu;
    QTimer m_timer;

    qreal m_cpuUsage = 0;
    qreal m_cpuTemp = -1;
    qreal m_gpuUsage = -1;
    qreal m_gpuTemp = -1;
    qreal m_vramUsage = -1;
    qreal m_memoryUsage = 0;
    qreal m_nvmeTemp = -1;

    QString m_hotSensor;
    QString m_candidate;
    qint64 m_candidateSince = 0; // ms since epoch
    int m_viewers = 0;
};

} // namespace island
