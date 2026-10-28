#include "statusnotifierwatcher.h"

#include "statusnotifierwatcheradaptor.h"
#include "trayparse.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcTrayWatcher, "island.tray.watcher")

using namespace Qt::StringLiterals;

namespace island {

namespace {
// Stands for the in-process host in m_hosts.
const auto kLocalHost = u"(island-shell)"_s;
} // namespace

StatusNotifierWatcher::StatusNotifierWatcher(QObject* parent) : QObject(parent) {
    new StatusNotifierWatcherAdaptor(this);
    m_watcher.setConnection(QDBusConnection::sessionBus());
    m_watcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(&m_watcher, &QDBusServiceWatcher::serviceUnregistered, this,
            &StatusNotifierWatcher::onServiceGone);
}

StatusNotifierWatcher::~StatusNotifierWatcher() {
    if (!m_registered)
        return;
    auto bus = QDBusConnection::sessionBus();
    bus.unregisterService(QLatin1String(kService));
    bus.unregisterObject(QLatin1String(kPath));
}

bool StatusNotifierWatcher::registerOnBus() {
    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return false;
    if (!bus.registerObject(QLatin1String(kPath), this))
        return false;
    // No queueing: when someone else has it, Tray uses theirs.
    const auto reply = bus.interface()->registerService(QLatin1String(kService),
                                                       QDBusConnectionInterface::DontQueueService,
                                                       QDBusConnectionInterface::DontAllowReplacement);
    if (!reply.isValid() || reply.value() != QDBusConnectionInterface::ServiceRegistered) {
        bus.unregisterObject(QLatin1String(kPath));
        return false;
    }
    m_registered = true;
    qCInfo(lcTrayWatcher) << "serving" << kService;
    return true;
}

void StatusNotifierWatcher::addLocalHost() {
    if (m_hosts.contains(kLocalHost))
        return;
    m_hosts.append(kLocalHost);
    emit StatusNotifierHostRegistered();
}

void StatusNotifierWatcher::RegisterStatusNotifierItem(const QString& service) {
    const auto sender = calledFromDBus() ? message().service() : QString();
    const auto address = parseTrayAddress(service, sender);
    if (address.service.isEmpty())
        return;
    const auto key = address.key();
    if (m_items.contains(key))
        return;
    m_items.append(key);
    m_watcher.addWatchedService(address.service);
    qCDebug(lcTrayWatcher) << "item" << key;
    emit StatusNotifierItemRegistered(key);
}

void StatusNotifierWatcher::RegisterStatusNotifierHost(const QString& service) {
    const auto name = service.isEmpty() && calledFromDBus() ? message().service() : service;
    if (name.isEmpty() || m_hosts.contains(name))
        return;
    m_hosts.append(name);
    m_watcher.addWatchedService(name);
    emit StatusNotifierHostRegistered();
}

void StatusNotifierWatcher::onServiceGone(const QString& service) {
    m_watcher.removeWatchedService(service);
    for (qsizetype i = m_items.size() - 1; i >= 0; --i) {
        if (parseTrayAddress(m_items[i]).service == service)
            emit StatusNotifierItemUnregistered(m_items.takeAt(i));
    }
    if (m_hosts.removeAll(service) > 0)
        emit StatusNotifierHostUnregistered();
}

} // namespace island
