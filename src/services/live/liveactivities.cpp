#include "liveactivities.h"

#include "liveactivitiesadaptor.h"

#include "dbusutil.h"

#include <QCoreApplication>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusVariant>
#include <QJSEngine>
#include <QLoggingCategory>
#include <QProcess>

#include <algorithm>
#include <utility>

#include "icons.h"

Q_LOGGING_CATEGORY(lcLive, "island.live")

namespace island {

namespace {

using dbus::toMap;
using dbus::toStringList;
using dbus::unwrap;

constexpr int kDefaultCliTtl = 30 * 60;

} // namespace

LiveActivities* LiveActivities::instance() {
    static auto* live = new LiveActivities(QCoreApplication::instance());
    return live;
}

LiveActivities* LiveActivities::create(QQmlEngine*, QJSEngine*) {
    auto* live = instance();
    QJSEngine::setObjectOwnership(live, QJSEngine::CppOwnership);
    return live;
}

LiveActivities::LiveActivities(QObject* parent) : QAbstractListModel(parent) {
    new LiveActivitiesAdaptor(this);
    m_watcher.setConnection(QDBusConnection::sessionBus());
    m_watcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(&m_watcher, &QDBusServiceWatcher::serviceUnregistered, this,
            &LiveActivities::onOwnerGone);
    connect(this, &QAbstractItemModel::rowsInserted, this, &LiveActivities::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &LiveActivities::countChanged);
    m_ttlTimer.setInterval(1000);
    connect(&m_ttlTimer, &QTimer::timeout, this, &LiveActivities::checkExpired);
}

void LiveActivities::registerOnBus() {
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/org/islandshell/LiveActivities"),
                                                 this);
}

int LiveActivities::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant LiveActivities::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_items.size())
        return {};
    const auto& it = m_items[index.row()];
    switch (role) {
    case LiveIdRole:
        return it.id;
    case TitleRole:
        return it.title;
    case SubtitleRole:
        return it.subtitle;
    case IconRole:
        return Icons::source(it.icon);
    case ProgressRole:
        return it.progress;
    case StateRole:
        return it.state;
    case ActionsRole: {
        QVariantList actions;
        for (const auto& a : it.actions) {
            const auto eq = a.indexOf(u'=');
            actions.append(QVariantMap{
                {QStringLiteral("key"), eq > 0 ? a.left(eq) : a},
                {QStringLiteral("label"), eq > 0 ? a.mid(eq + 1) : a},
            });
        }
        return actions;
    }
    case AlertSerialRole:
        return it.alertSerial;
    default:
        return {};
    }
}

QHash<int, QByteArray> LiveActivities::roleNames() const {
    return {
        {LiveIdRole, "liveId"},     {TitleRole, "title"},     {SubtitleRole, "subtitle"},
        {IconRole, "icon"},         {ProgressRole, "progress"}, {StateRole, "state"},
        {ActionsRole, "actions"},   {AlertSerialRole, "alertSerial"},
    };
}

int LiveActivities::indexOf(const QString& id) const {
    for (int i = 0; i < m_items.size(); ++i)
        if (m_items[i].id == id)
            return i;
    return -1;
}

void LiveActivities::fail(const QString& message) {
    if (calledFromDBus())
        sendErrorReply(QStringLiteral("org.islandshell.Error.InvalidArgs"), message);
    else
        qCWarning(lcLive).noquote() << message;
}

bool LiveActivities::apply(Item& item, const QVariantMap& props, QString* error) {
    for (auto it = props.cbegin(); it != props.cend(); ++it) {
        const auto& key = it.key();
        const auto value = unwrap(it.value());
        if (key == u"title")
            item.title = value.toString();
        else if (key == u"subtitle")
            item.subtitle = value.toString();
        else if (key == u"icon")
            item.icon = value.toString();
        else if (key == u"progress")
            item.progress = std::min(value.toDouble(), 1.0);
        else if (key == u"state") {
            const auto s = value.toString();
            if (s != u"running" && s != u"success" && s != u"error") {
                *error = QStringLiteral("state must be running, success or error");
                return false;
            }
            if (s != item.state)
                ++item.alertSerial;
            item.state = s;
        } else if (key == u"ttl") {
            const int ttl = value.toInt();
            item.expires = ttl > 0 ? QDateTime::currentDateTime().addSecs(ttl) : QDateTime();
        } else if (key == u"actions")
            item.actions = toStringList(it.value());
        else if (key == u"commands")
            item.commands = toMap(it.value());
        else if (key == u"persist") {
            // handled by the caller
        } else {
            *error = QStringLiteral("unknown prop \"%1\"").arg(key);
            return false;
        }
    }
    return true;
}

void LiveActivities::Push(const QString& id, const QVariantMap& props) {
    if (id.isEmpty()) {
        fail(QStringLiteral("id must not be empty"));
        return;
    }
    Item item;
    item.id = id;
    QString error;
    if (!apply(item, props, &error)) {
        fail(error);
        return;
    }
    const bool persist = unwrap(props.value(QStringLiteral("persist"))).toBool();
    if (!persist && calledFromDBus()) {
        item.owner = message().service();
        m_watcher.addWatchedService(item.owner);
    }
    if (persist && !item.expires.isValid() && !props.contains(QStringLiteral("ttl")))
        item.expires = QDateTime::currentDateTime().addSecs(kDefaultCliTtl);

    const int row = indexOf(id);
    if (row >= 0) {
        item.alertSerial = m_items[row].alertSerial + 1;
        m_items[row] = item;
        emit dataChanged(index(row), index(row));
    } else {
        item.alertSerial = 1;
        beginInsertRows({}, static_cast<int>(m_items.size()), static_cast<int>(m_items.size()));
        m_items.append(item);
        endInsertRows();
    }
    if (item.expires.isValid())
        m_ttlTimer.start();
}

void LiveActivities::set(const QString& id, const QVariantMap& props, const QString& owner) {
    const int row = indexOf(id);
    if (row >= 0) {
        Update(id, props);
        return;
    }
    Item item;
    item.id = id;
    QString error;
    if (!apply(item, props, &error)) {
        qCWarning(lcLive).noquote() << id << error;
        return;
    }
    item.owner = owner;
    if (!owner.isEmpty())
        m_watcher.addWatchedService(owner);
    item.alertSerial = 1;
    beginInsertRows({}, static_cast<int>(m_items.size()), static_cast<int>(m_items.size()));
    m_items.append(item);
    endInsertRows();
    if (item.expires.isValid())
        m_ttlTimer.start();
}

void LiveActivities::disown(const QString& id) {
    const int row = indexOf(id);
    if (row < 0)
        return;
    const auto owner = std::exchange(m_items[row].owner, QString());
    if (!owner.isEmpty() && std::ranges::none_of(m_items, [&](const Item& i) { return i.owner == owner; }))
        m_watcher.removeWatchedService(owner);
}

void LiveActivities::Update(const QString& id, const QVariantMap& props) {
    const int row = indexOf(id);
    if (row < 0) {
        fail(QStringLiteral("no live activity \"%1\"").arg(id));
        return;
    }
    Item updated = m_items[row];
    QString error;
    if (!apply(updated, props, &error)) {
        fail(error);
        return;
    }
    m_items[row] = updated;
    emit dataChanged(index(row), index(row));
    if (updated.expires.isValid())
        m_ttlTimer.start();
}

void LiveActivities::End(const QString& id) {
    const int row = indexOf(id);
    if (row < 0) {
        fail(QStringLiteral("no live activity \"%1\"").arg(id));
        return;
    }
    endIndex(row);
}

QStringList LiveActivities::List() {
    QStringList ids;
    for (const auto& it : std::as_const(m_items))
        ids.append(it.id);
    return ids;
}

void LiveActivities::endIndex(int row) {
    beginRemoveRows({}, row, row);
    const auto owner = m_items.takeAt(row).owner;
    endRemoveRows();
    if (!owner.isEmpty() &&
        std::ranges::none_of(m_items, [&](const Item& i) { return i.owner == owner; }))
        m_watcher.removeWatchedService(owner);
}

void LiveActivities::checkExpired() {
    const auto now = QDateTime::currentDateTime();
    bool any = false;
    for (int i = static_cast<int>(m_items.size()) - 1; i >= 0; --i) {
        if (m_items[i].expires.isValid()) {
            any = true;
            if (m_items[i].expires <= now)
                endIndex(i);
        }
    }
    if (!any)
        m_ttlTimer.stop();
}

void LiveActivities::onOwnerGone(const QString& service) {
    for (int i = static_cast<int>(m_items.size()) - 1; i >= 0; --i)
        if (m_items[i].owner == service)
            endIndex(i);
}

void LiveActivities::invoke(const QString& id, const QString& key) {
    const int row = indexOf(id);
    if (row < 0)
        return;
    const auto command = m_items[row].commands.value(key).toString();
    if (!command.isEmpty())
        QProcess::startDetached(QStringLiteral("sh"), {QStringLiteral("-c"), command});
    emit ActionInvoked(id, key);
}

} // namespace island
