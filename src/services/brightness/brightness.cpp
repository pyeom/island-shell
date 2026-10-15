#include "brightness.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QJSEngine>

#include <cmath>

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
    m_fake = !m_dir.isEmpty();
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

void Brightness::setLevel(qreal level) {
    const qint64 max = readNumber(m_dir + QLatin1String("/max_brightness"));
    if (max <= 0)
        return;
    // Never fully dark: 1% minimum so the screen stays usable.
    const auto value = static_cast<uint>(std::round(std::clamp(level, 0.01, 1.0) * max));
    if (m_fake) {
        QFile f(m_dir + QLatin1String("/actual_brightness"));
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            f.write(QByteArray::number(value));
        return;
    }
    auto msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.login1"), QStringLiteral("/org/freedesktop/login1/session/auto"),
        QStringLiteral("org.freedesktop.login1.Session"), QStringLiteral("SetBrightness"));
    msg.setArguments({QStringLiteral("backlight"), QDir(m_dir).dirName(), value});
    QDBusConnection::systemBus().asyncCall(msg);
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
