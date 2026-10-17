#include "systemstats.h"

#include "config.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJSEngine>

#include <algorithm>
#include <iterator>

namespace island {

namespace {

QByteArray readFile(const QString& path) {
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

qreal readNumber(const QString& path, qreal scale = 1) {
    if (path.isEmpty())
        return -1;
    bool ok = false;
    const qreal v = readFile(path).trimmed().toDouble(&ok);
    return ok ? v / scale : -1;
}

// First temperature input of the hwmon device whose name is in `names`,
// preferring an input whose label is in `labels`.
QString hwmonTemp(const QStringList& names, const QStringList& labels = {}) {
    const QDir root(QStringLiteral("/sys/class/hwmon"));
    for (const auto& entry : root.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System)) {
        const auto dir = root.filePath(entry);
        if (!names.contains(QString::fromUtf8(readFile(dir + QLatin1String("/name")).trimmed())))
            continue;
        const auto inputs = QDir(dir).entryList({QStringLiteral("temp*_input")}, QDir::Files, QDir::Name);
        for (const auto& input : inputs) {
            const auto label = QString::fromUtf8(
                readFile(dir + u'/' + QString(input).replace(QLatin1String("_input"), QLatin1String("_label"))).trimmed());
            if (labels.isEmpty() || labels.contains(label))
                return dir + u'/' + input;
        }
        if (!inputs.isEmpty())
            return dir + u'/' + inputs.first();
    }
    return {};
}

} // namespace

SystemStats* SystemStats::create(QQmlEngine*, QJSEngine*) {
    static auto* stats = new SystemStats(QCoreApplication::instance());
    QJSEngine::setObjectOwnership(stats, QJSEngine::CppOwnership);
    return stats;
}

SystemStats::SystemStats(QObject* parent) : QObject(parent) {
    findSensors();
    if (auto t = sys::parseProcStat(readFile(QStringLiteral("/proc/stat"))))
        m_lastCpu = *t;
    connect(&m_timer, &QTimer::timeout, this, &SystemStats::sample);
    sample(); // starts the timer
}

void SystemStats::findSensors() {
    // AMD: k10temp (Tctl/Tdie); Intel: coretemp (Package); ARM: cpu_thermal.
    m_cpuTempPath = hwmonTemp({QStringLiteral("k10temp"), QStringLiteral("zenpower"),
                               QStringLiteral("coretemp"), QStringLiteral("cpu_thermal")},
                              {QStringLiteral("Tdie"), QStringLiteral("Tctl"),
                               QStringLiteral("Package id 0")});
    m_gpuTempPath = hwmonTemp({QStringLiteral("amdgpu")}, {QStringLiteral("edge")});
    m_nvmeTempPath = hwmonTemp({QStringLiteral("nvme")}, {QStringLiteral("Composite")});
    const QDir drm(QStringLiteral("/sys/class/drm"));
    for (const auto& card : drm.entryList({QStringLiteral("card?")}, QDir::Dirs | QDir::System)) {
        const auto dir = drm.filePath(card) + QLatin1String("/device");
        if (QFile::exists(dir + QLatin1String("/gpu_busy_percent"))) {
            m_gpuDir = dir;
            break;
        }
    }
}

void SystemStats::addViewer() {
    if (m_viewers++ == 0) {
        sample(); // fresh values right away, not up to 10 s old
        updateInterval();
    }
}

void SystemStats::removeViewer() {
    m_viewers = std::max(0, m_viewers - 1);
    updateInterval();
}

void SystemStats::updateInterval() {
    // Fast while shown, or while a temperature is over its limit (the alert
    // needs 10 s of it) or the alert is on (it shows the temperature).
    const int interval = m_viewers > 0 || !m_candidate.isEmpty() || hot() ? 2000 : 10000;
    if (m_timer.interval() != interval)
        m_timer.start(interval);
}

void SystemStats::sample() {
    const qreal before[] = {m_cpuUsage, m_memoryUsage, m_gpuUsage, m_vramUsage,
                            m_cpuTemp,  m_gpuTemp,     m_nvmeTemp};
    m_cpuTemp = readNumber(m_cpuTempPath, 1000);
    m_gpuTemp = readNumber(m_gpuTempPath, 1000);
    m_nvmeTemp = readNumber(m_nvmeTempPath, 1000);
    if (m_viewers > 0) {
        if (auto t = sys::parseProcStat(readFile(QStringLiteral("/proc/stat")))) {
            m_cpuUsage = sys::cpuUsage(m_lastCpu, *t);
            m_lastCpu = *t;
        }
        if (auto m = sys::parseMeminfo(readFile(QStringLiteral("/proc/meminfo"))))
            m_memoryUsage = m->usedFraction();
        if (!m_gpuDir.isEmpty()) {
            const qreal busy = readNumber(m_gpuDir + QLatin1String("/gpu_busy_percent"));
            m_gpuUsage = busy >= 0 ? busy / 100 : -1;
            const qreal used = readNumber(m_gpuDir + QLatin1String("/mem_info_vram_used"));
            const qreal total = readNumber(m_gpuDir + QLatin1String("/mem_info_vram_total"));
            m_vramUsage = used >= 0 && total > 0 ? used / total : -1;
        }
    }
    const qreal after[] = {m_cpuUsage, m_memoryUsage, m_gpuUsage, m_vramUsage,
                           m_cpuTemp,  m_gpuTemp,     m_nvmeTemp};
    if (!std::equal(std::begin(before), std::end(before), std::begin(after)))
        emit changed();
    checkHeat();
    updateInterval();
}

qreal SystemStats::hotTemp() const {
    if (m_hotSensor == u"cpu")
        return m_cpuTemp;
    if (m_hotSensor == u"gpu")
        return m_gpuTemp;
    if (m_hotSensor == u"nvme")
        return m_nvmeTemp;
    return -1;
}

void SystemStats::checkHeat() {
    const auto& cfg = Config::instance()->data().system;
    struct Sensor {
        const char* name;
        qreal temp;
        int limit;
    };
    const Sensor sensors[] = {{"cpu", m_cpuTemp, cfg.cpuHot},
                              {"gpu", m_gpuTemp, cfg.gpuHot},
                              {"nvme", m_nvmeTemp, cfg.nvmeHot}};

    if (!m_hotSensor.isEmpty()) {
        // Off 5 °C below the threshold, so it does not flap at the edge.
        for (const auto& s : sensors)
            if (m_hotSensor == QLatin1String(s.name) && (s.temp < 0 || s.temp < s.limit - 5)) {
                m_hotSensor.clear();
                emit hotChanged();
            }
        return;
    }
    QString over;
    for (const auto& s : sensors)
        if (s.limit > 0 && s.temp >= s.limit) {
            over = QLatin1String(s.name);
            break;
        }
    const auto now = QDateTime::currentMSecsSinceEpoch();
    if (over.isEmpty() || over != m_candidate) {
        m_candidate = over;
        m_candidateSince = now;
        return;
    }
    // Sustained for 10 s.
    if (now - m_candidateSince >= 10000) {
        m_hotSensor = over;
        m_candidate.clear();
        emit hotChanged();
    }
}

} // namespace island
