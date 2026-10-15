#include "brightness.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJSEngine>

namespace island {

namespace {
qint64 readNumber(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return -1;
    bool ok = false;
    const qint64 v = f.readAll().trimmed().toLongLong(&ok);
    return ok ? v : -1;
}
} // namespace

Brightness* Brightness::create(QQmlEngine*, QJSEngine*) {
    static auto* b = new Brightness(QCoreApplication::instance());
    QJSEngine::setObjectOwnership(b, QJSEngine::CppOwnership);
    return b;
}

Brightness::Brightness(QObject* parent) : QObject(parent) {
    m_dir = qEnvironmentVariable("ISLAND_SHELL_BACKLIGHT");
    if (m_dir.isEmpty()) {
        const QDir sys(QStringLiteral("/sys/class/backlight"));
        const auto devices = sys.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        if (!devices.isEmpty())
            m_dir = sys.filePath(devices.first());
    }
    if (m_dir.isEmpty())
        return;
    m_poll.setInterval(250);
    connect(&m_poll, &QTimer::timeout, this, &Brightness::poll);
    m_poll.start();
    poll();
}

void Brightness::poll() {
    const qint64 max = readNumber(m_dir + QLatin1String("/max_brightness"));
    qint64 cur = readNumber(m_dir + QLatin1String("/actual_brightness"));
    if (cur < 0)
        cur = readNumber(m_dir + QLatin1String("/brightness"));
    if (max <= 0 || cur < 0)
        return;
    const qreal level = static_cast<qreal>(cur) / static_cast<qreal>(max);
    if (std::abs(level - m_level) < 0.001)
        return;
    const bool first = m_level < 0;
    m_level = level;
    if (!first)
        emit changed();
}

} // namespace island
