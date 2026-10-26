#include "timers.h"

#include <QCoreApplication>
#include <QFile>
#include <QJSEngine>
#include <QProcess>
#include <QStandardPaths>

#include <algorithm>

namespace island {

namespace {
// Finished timers leave on their own after this long.
constexpr qint64 kDoneLingerMs = 10 * 60 * 1000;
} // namespace

Timers* Timers::instance() {
    static auto* t = new Timers(QCoreApplication::instance());
    return t;
}

Timers* Timers::create(QQmlEngine*, QJSEngine*) {
    auto* t = instance();
    QJSEngine::setObjectOwnership(t, QJSEngine::CppOwnership);
    return t;
}

Timers::Timers(QObject* parent) : QObject(parent) {
    m_next.setSingleShot(true);
    // Wall-clock deadlines: after a suspend the timer fires late and check()
    // finds everything that ended meanwhile.
    m_next.setTimerType(Qt::PreciseTimer);
    connect(&m_next, &QTimer::timeout, this, &Timers::check);
}

QVariantList Timers::timers() const {
    auto sorted = m_entries;
    std::ranges::stable_sort(sorted, [](const Entry& a, const Entry& b) {
        if (a.done != b.done)
            return a.done;
        return a.endsAt < b.endsAt;
    });
    QVariantList out;
    for (const auto& e : sorted) {
        out.append(QVariantMap{
            {QStringLiteral("id"), e.id},
            {QStringLiteral("label"), e.label},
            {QStringLiteral("duration"), e.duration},
            {QStringLiteral("endsAt"), e.endsAt},
            {QStringLiteral("done"), e.done},
        });
    }
    return out;
}

int Timers::start(int seconds, const QString& label) {
    if (seconds <= 0)
        return 0;
    Entry e;
    e.id = ++m_lastId;
    e.label = label.trimmed();
    e.duration = seconds;
    e.endsAt = QDateTime::currentDateTime().addSecs(seconds);
    m_entries.append(e);
    schedule();
    emit changed();
    return e.id;
}

void Timers::extend(int id, int seconds) {
    const auto it = std::ranges::find(m_entries, id, &Entry::id);
    if (it == m_entries.end() || seconds <= 0)
        return;
    if (it->done) {
        it->done = false;
        it->duration = seconds;
        it->endsAt = QDateTime::currentDateTime().addSecs(seconds);
    } else {
        it->duration += seconds;
        it->endsAt = it->endsAt.addSecs(seconds);
    }
    schedule();
    emit changed();
}

void Timers::remove(int id) {
    if (m_entries.removeIf([id](const Entry& e) { return e.id == id; }) == 0)
        return;
    schedule();
    emit changed();
}

void Timers::schedule() {
    const auto now = QDateTime::currentDateTime();
    qint64 next = -1;
    for (const auto& e : std::as_const(m_entries)) {
        const qint64 ms = e.done ? now.msecsTo(e.endsAt) + kDoneLingerMs : now.msecsTo(e.endsAt);
        if (next < 0 || ms < next)
            next = ms;
    }
    if (next < 0)
        m_next.stop();
    else
        m_next.start(static_cast<int>(std::clamp<qint64>(next, 0, 60 * 60 * 1000)));
}

void Timers::check() {
    const auto now = QDateTime::currentDateTime();
    bool changedAny = false;
    QList<Entry> rang;
    for (auto& e : m_entries) {
        if (!e.done && e.endsAt <= now) {
            e.done = true;
            rang.append(e);
            changedAny = true;
        }
    }
    changedAny |= m_entries.removeIf([&](const Entry& e) {
        return e.done && e.endsAt.msecsTo(now) >= kDoneLingerMs;
    }) > 0;
    schedule();
    if (changedAny)
        emit changed();
    if (!rang.isEmpty())
        ring();
    for (const auto& e : rang)
        emit finished(e.id, e.label);
}

void Timers::ring() {
    const auto sound = QStringLiteral("/usr/share/sounds/freedesktop/stereo/alarm-clock-elapsed.oga");
    if (QFile::exists(sound) && !QStandardPaths::findExecutable(QStringLiteral("pw-play")).isEmpty())
        QProcess::startDetached(QStringLiteral("pw-play"), {sound});
    else if (!QStandardPaths::findExecutable(QStringLiteral("canberra-gtk-play")).isEmpty())
        QProcess::startDetached(QStringLiteral("canberra-gtk-play"),
                                {QStringLiteral("-i"), QStringLiteral("alarm-clock-elapsed")});
}

} // namespace island
