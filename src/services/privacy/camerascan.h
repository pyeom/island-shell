#pragma once

#include <QString>
#include <QStringList>

namespace island {

// Who holds a camera open: names (comm) of processes under `procRoot` with
// a /dev/video* descriptor, sorted, without duplicates. Skips `selfPid`.
QStringList cameraUsers(const QString& procRoot = QStringLiteral("/proc"), qint64 selfPid = 0);

// inotify events on /dev/video* → whether to look in /proc again.
// Programs probe cameras by opening and closing them right away (something
// on a desktop can do it every few seconds); when every open was closed
// again and nobody used a camera before, nothing changed.
class CameraActivity {
public:
    void opened() { ++m_balance; }
    void closed() { --m_balance; }
    void overflowed() { m_overflow = true; }
    bool needsScan(bool hadUsers) const { return m_overflow || m_balance > 0 || hadUsers; }
    void scanned() {
        m_balance = 0;
        m_overflow = false;
    }

private:
    int m_balance = 0; // opens minus closes since the last scan
    bool m_overflow = false;
};

} // namespace island
