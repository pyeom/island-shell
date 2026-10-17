#include "camerascan.h"

#include <QFile>
#include <QSet>

#include <string>
#include <string_view>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

namespace island {

namespace {

bool isNumber(const char* s) {
    if (!*s)
        return false;
    for (; *s; ++s) {
        if (*s < '0' || *s > '9')
            return false;
    }
    return true;
}

// Any descriptor of the process pointing at /dev/video*. Plain
// opendir/readlinkat: thousands of descriptors per scan, and QDir/QFileInfo
// allocate for every one.
bool holdsCamera(int procFd, const char* pid) {
    const int fdDir = openat(procFd, (std::string(pid) + "/fd").c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fdDir < 0)
        return false; // gone, or another user's
    DIR* dir = fdopendir(fdDir);
    if (!dir) {
        close(fdDir);
        return false;
    }
    constexpr std::string_view prefix = "/dev/video";
    bool found = false;
    char target[64];
    while (const dirent* e = readdir(dir)) {
        if (e->d_name[0] == '.')
            continue;
        const ssize_t n = readlinkat(fdDir, e->d_name, target, sizeof(target) - 1);
        if (n >= static_cast<ssize_t>(prefix.size()) && std::string_view(target, prefix.size()) == prefix) {
            found = true;
            break;
        }
    }
    closedir(dir);
    return found;
}

} // namespace

QStringList cameraUsers(const QString& procRoot, qint64 selfPid) {
    const int procFd = open(QFile::encodeName(procRoot).constData(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (procFd < 0)
        return {};
    DIR* proc = fdopendir(procFd);
    if (!proc) {
        close(procFd);
        return {};
    }
    const auto self = QByteArray::number(selfPid);
    QSet<QString> names;
    while (const dirent* e = readdir(proc)) {
        if (!isNumber(e->d_name) || (selfPid > 0 && self == e->d_name) || !holdsCamera(procFd, e->d_name))
            continue;
        QFile comm(procRoot + u'/' + QString::fromLatin1(e->d_name) + QLatin1String("/comm"));
        if (comm.open(QIODevice::ReadOnly))
            names.insert(QString::fromUtf8(comm.readAll()).trimmed());
    }
    closedir(proc);
    QStringList list(names.begin(), names.end());
    list.removeAll(QString());
    list.sort(Qt::CaseInsensitive);
    return list;
}

} // namespace island
