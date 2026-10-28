#include "tray.h"

#include "dbusproperties.h"
#include "dbusutil.h"
#include "icons.h"
#include "statusnotifierwatcher.h"
#include "trayparse.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QDBusVariant>
#include <QDirIterator>
#include <QFileInfo>
#include <QJSEngine>
#include <QLoggingCategory>
#include <QTextDocumentFragment>

Q_LOGGING_CATEGORY(lcTray, "island.tray")

using namespace Qt::StringLiterals;

namespace island {

namespace {

const auto kItemInterface = u"org.kde.StatusNotifierItem"_s;
const auto kWatcherInterface = u"org.kde.StatusNotifierWatcher"_s;
constexpr int kPixmapSize = 64;

// Forwards an item's New* signals (they carry nothing useful) to a callback.
class SignalRelay : public QObject {
    Q_OBJECT
public:
    SignalRelay(std::function<void()> fn, QObject* parent) : QObject(parent), m_fn(std::move(fn)) {}
public slots:
    void fire() { m_fn(); }

private:
    std::function<void()> m_fn;
};

QList<TrayPixmap> readPixmaps(const QDBusArgument& arg) {
    QList<TrayPixmap> out;
    arg.beginArray();
    while (!arg.atEnd()) {
        TrayPixmap p;
        arg.beginStructure();
        arg >> p.width >> p.height >> p.data;
        arg.endStructure();
        out.append(p);
    }
    arg.endArray();
    return out;
}

QList<TrayPixmap> toPixmaps(const QVariant& v) {
    const auto inner = dbus::unwrap(v);
    if (!inner.canConvert<QDBusArgument>())
        return {};
    return readPixmaps(inner.value<QDBusArgument>());
}

// ToolTip: (icon name, icon pixmaps, title, text).
std::pair<QString, QString> toTooltip(const QVariant& v) {
    const auto inner = dbus::unwrap(v);
    if (!inner.canConvert<QDBusArgument>())
        return {};
    const auto arg = inner.value<QDBusArgument>();
    QString icon, title, text;
    arg.beginStructure();
    arg >> icon;
    readPixmaps(arg);
    arg >> title >> text;
    arg.endStructure();
    // The text may be HTML.
    return {title, QTextDocumentFragment::fromHtml(text).toPlainText().trimmed()};
}

// An icon name, looked up in the app's own IconThemePath first.
QString namedIcon(const QString& name, const QString& themePath) {
    if (name.isEmpty())
        return {};
    if (name.startsWith(u'/'))
        return QFileInfo::exists(name) ? Icons::source(name) : QString();
    if (!themePath.isEmpty() && QFileInfo(themePath).isDir()) {
        const QStringList filters{name, name + u".svg"_s, name + u".png"_s, name + u".xpm"_s};
        QDirIterator it(themePath, filters, QDir::Files, QDirIterator::Subdirectories);
        if (it.hasNext())
            return Icons::source(it.next());
    }
    return Icons::exists(name) ? Icons::source(name) : QString();
}

bool hasMenuPath(const QString& path) {
    return !path.isEmpty() && path != u"/"_s && path != u"/NO_DBUSMENU"_s;
}

} // namespace

// ---------------------------------------------------------------------------

QMutex TrayImageProvider::s_mutex;
QHash<QString, QImage> TrayImageProvider::s_images;
quint64 TrayImageProvider::s_next = 0;

TrayImageProvider::TrayImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

QImage TrayImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize) {
    QImage image;
    {
        QMutexLocker lock(&s_mutex);
        image = s_images.value(id);
    }
    if (!image.isNull() && requestedSize.isValid() && !requestedSize.isEmpty())
        image = image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (size)
        *size = image.size();
    return image;
}

QString TrayImageProvider::store(const QImage& image, const QString& previous) {
    drop(previous);
    QMutexLocker lock(&s_mutex);
    const auto id = QString::number(++s_next);
    s_images.insert(id, image);
    return u"image://tray/"_s + id;
}

void TrayImageProvider::drop(const QString& source) {
    static const auto prefix = u"image://tray/"_s;
    if (!source.startsWith(prefix))
        return;
    QMutexLocker lock(&s_mutex);
    s_images.remove(source.mid(prefix.size()));
}

// ---------------------------------------------------------------------------

Tray* Tray::instance() {
    static auto* tray = new Tray(QCoreApplication::instance());
    return tray;
}

Tray* Tray::create(QQmlEngine*, QJSEngine*) {
    auto* tray = instance();
    QJSEngine::setObjectOwnership(tray, QJSEngine::CppOwnership);
    return tray;
}

Tray::Tray(QObject* parent) : QAbstractListModel(parent) {
    m_refresh.setSingleShot(true);
    m_refresh.setInterval(50); // apps send NewIcon, NewToolTip... in bursts
    connect(&m_refresh, &QTimer::timeout, this, &Tray::refreshDirty);
}

void Tray::start() {
    if (m_started)
        return;
    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return;
    m_started = true;
    m_hostName = u"org.kde.StatusNotifierHost-%1"_s.arg(QCoreApplication::applicationPid());
    bus.registerService(m_hostName);

    // Another program's watcher. Ours emits the same signals locally.
    const auto service = QLatin1String(StatusNotifierWatcher::kService);
    const auto path = QLatin1String(StatusNotifierWatcher::kPath);
    bus.connect(service, path, kWatcherInterface, u"StatusNotifierItemRegistered"_s, this,
                SLOT(onItemRegistered(QString)));
    bus.connect(service, path, kWatcherInterface, u"StatusNotifierItemUnregistered"_s, this,
                SLOT(onItemUnregistered(QString)));
    auto* owner = new QDBusServiceWatcher(service, bus, QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(owner, &QDBusServiceWatcher::serviceOwnerChanged, this,
            [this](const QString&, const QString&, const QString& newOwner) {
                if (m_watcher)
                    return; // ours
                reset();
                // Theirs quit (waybar closed): serve it ourselves.
                if (newOwner.isEmpty())
                    useOwnWatcher();
                if (!m_watcher && !newOwner.isEmpty())
                    useWatcher(newOwner);
            });

    useOwnWatcher();
    if (!m_watcher)
        useWatcher({});
}

void Tray::useOwnWatcher() {
    auto* watcher = new StatusNotifierWatcher(this);
    if (!watcher->registerOnBus()) {
        delete watcher;
        return;
    }
    m_watcher = watcher;
    connect(watcher, &StatusNotifierWatcher::StatusNotifierItemRegistered, this, &Tray::addItem);
    connect(watcher, &StatusNotifierWatcher::StatusNotifierItemUnregistered, this, &Tray::removeItem);
    watcher->addLocalHost();
    setAvailable(true);
}

void Tray::useWatcher(const QString& owner) {
    qCInfo(lcTray).noquote() << "using the tray watcher of" << (owner.isEmpty() ? u"another program"_s : owner);
    auto bus = QDBusConnection::sessionBus();
    const auto service = QLatin1String(StatusNotifierWatcher::kService);
    const auto path = QLatin1String(StatusNotifierWatcher::kPath);

    auto host = QDBusMessage::createMethodCall(service, path, kWatcherInterface, u"RegisterStatusNotifierHost"_s);
    host.setArguments({m_hostName});
    bus.asyncCall(host);

    auto get = QDBusMessage::createMethodCall(service, path, u"org.freedesktop.DBus.Properties"_s, u"Get"_s);
    get.setArguments({kWatcherInterface, u"RegisteredStatusNotifierItems"_s});
    auto* call = new QDBusPendingCallWatcher(bus.asyncCall(get), this);
    connect(call, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* w) {
        w->deleteLater();
        QDBusPendingReply<QDBusVariant> reply = *w;
        if (reply.isError()) {
            qCWarning(lcTray).noquote() << "no tray watcher:" << reply.error().message();
            setAvailable(false);
            return;
        }
        setAvailable(true);
        for (const auto& item : dbus::toStringList(reply.value().variant()))
            addItem(item);
    });
}

void Tray::onItemRegistered(const QString& service) {
    if (!m_watcher)
        addItem(service);
}

void Tray::onItemUnregistered(const QString& service) {
    if (!m_watcher)
        removeItem(service);
}

void Tray::setAvailable(bool available) {
    if (available == m_available)
        return;
    m_available = available;
    emit availableChanged();
}

void Tray::reset() {
    beginResetModel();
    for (auto& item : m_items) {
        TrayImageProvider::drop(item.pixmapSource);
        delete item.props;
    }
    m_items.clear();
    m_dirty.clear();
    endResetModel();
    setAvailable(false);
    recount();
}

int Tray::indexOf(const QString& key) const {
    for (int i = 0; i < m_items.size(); ++i)
        if (m_items[i].key == key)
            return i;
    return -1;
}

void Tray::addItem(const QString& registered) {
    const auto address = parseTrayAddress(registered);
    const auto key = address.key();
    if (address.service.isEmpty() || indexOf(key) >= 0)
        return;
    auto bus = QDBusConnection::sessionBus();
    Item item;
    item.key = key;
    item.service = address.service;
    item.path = address.path;
    item.props = new DBusProperties(bus, item.service, item.path, kItemInterface, this);
    connect(item.props, &DBusProperties::changed, this, [this, key] { update(key); });

    auto* relay = new SignalRelay(
        [this, key] {
            if (!m_dirty.contains(key))
                m_dirty.append(key);
            m_refresh.start();
        },
        item.props);
    for (const auto* name : {"NewTitle", "NewIcon", "NewAttentionIcon", "NewOverlayIcon", "NewToolTip",
                             "NewStatus", "NewMenu"})
        bus.connect(item.service, item.path, kItemInterface, QLatin1String(name), relay, SLOT(fire()));

    qCDebug(lcTray) << "item" << key;
    beginInsertRows({}, static_cast<int>(m_items.size()), static_cast<int>(m_items.size()));
    m_items.append(item);
    endInsertRows();
}

void Tray::removeItem(const QString& registered) {
    const int row = indexOf(parseTrayAddress(registered).key());
    if (row < 0)
        return;
    beginRemoveRows({}, row, row);
    auto item = m_items.takeAt(row);
    endRemoveRows();
    TrayImageProvider::drop(item.pixmapSource);
    delete item.props;
    m_dirty.removeAll(item.key);
    recount();
}

void Tray::refreshDirty() {
    for (const auto& key : std::exchange(m_dirty, {})) {
        const int row = indexOf(key);
        if (row >= 0 && m_items[row].props)
            m_items[row].props->refresh();
    }
}

void Tray::update(const QString& key) {
    const int row = indexOf(key);
    if (row < 0 || !m_items[row].props || !m_items[row].props->ready())
        return;
    auto& item = m_items[row];
    const auto* p = item.props.data();
    const auto str = [p](const char* name) { return dbus::unwrap(p->value(QLatin1String(name))).toString(); };

    item.itemId = str("Id");
    const auto [tipTitle, tipText] = toTooltip(p->value(u"ToolTip"_s));
    item.title = str("Title");
    if (item.title.isEmpty())
        item.title = tipTitle.isEmpty() ? item.itemId : tipTitle;
    item.tooltip = tipText.isEmpty() && tipTitle != item.title ? tipTitle : tipText;
    const auto status = str("Status");
    item.status = status.isEmpty() ? u"Active"_s : status;
    item.itemIsMenu = dbus::unwrap(p->value(u"ItemIsMenu"_s)).toBool();
    const auto menu = dbus::toPath(p->value(u"Menu"_s));
    item.menuPath = hasMenuPath(menu) ? menu : QString();

    // Named icons before pixels, the attention icon while it wants attention.
    const auto themePath = str("IconThemePath");
    QString icon;
    QImage pixels;
    if (item.status == u"NeedsAttention") {
        icon = namedIcon(str("AttentionIconName"), themePath);
        if (icon.isEmpty())
            pixels = trayPixmapImage(toPixmaps(p->value(u"AttentionIconPixmap"_s)), kPixmapSize);
    }
    if (icon.isEmpty() && pixels.isNull())
        icon = namedIcon(str("IconName"), themePath);
    if (icon.isEmpty() && pixels.isNull())
        pixels = trayPixmapImage(toPixmaps(p->value(u"IconPixmap"_s)), kPixmapSize);
    if (!pixels.isNull()) {
        item.pixmapSource = TrayImageProvider::store(pixels, item.pixmapSource);
        icon = item.pixmapSource;
    } else {
        TrayImageProvider::drop(std::exchange(item.pixmapSource, {}));
    }
    item.icon = icon.isEmpty() ? Icons::source(u"application-x-executable"_s) : icon;
    item.ready = true;

    emit dataChanged(index(row), index(row));
    recount();
}

void Tray::recount() {
    const int n = static_cast<int>(std::ranges::count_if(m_items, [](const Item& i) { return i.shown(); }));
    if (n == m_count)
        return;
    m_count = n;
    emit countChanged();
}

int Tray::count() const {
    return m_count;
}

int Tray::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant Tray::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_items.size())
        return {};
    const auto& it = m_items[index.row()];
    switch (role) {
    case KeyRole:
        return it.key;
    case ItemIdRole:
        return it.itemId;
    case TitleRole:
        return it.title;
    case TooltipRole:
        return it.tooltip;
    case StatusRole:
        return it.status;
    case ShownRole:
        return it.shown();
    case IconRole:
        return it.icon;
    case HasMenuRole:
        return !it.menuPath.isEmpty();
    case ItemIsMenuRole:
        return it.itemIsMenu;
    case ServiceRole:
        return it.service;
    case MenuPathRole:
        return it.menuPath;
    default:
        return {};
    }
}

QHash<int, QByteArray> Tray::roleNames() const {
    return {
        {KeyRole, "key"},         {ItemIdRole, "itemId"},     {TitleRole, "title"},
        {TooltipRole, "tooltip"}, {StatusRole, "status"},     {ShownRole, "shown"},
        {IconRole, "icon"},       {HasMenuRole, "hasMenu"},   {ItemIsMenuRole, "itemIsMenu"},
        {ServiceRole, "service"}, {MenuPathRole, "menuPath"},
    };
}

void Tray::call(const QString& key, const QString& method, const QVariantList& args,
                const std::function<void(bool)>& done) {
    const int row = indexOf(key);
    if (row < 0)
        return;
    auto msg = QDBusMessage::createMethodCall(m_items[row].service, m_items[row].path, kItemInterface, method);
    msg.setArguments(args);
    auto* watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [key, method, done](QDBusPendingCallWatcher* w) {
        w->deleteLater();
        if (w->isError())
            qCDebug(lcTray).noquote() << key << method << "->" << w->error().message();
        if (done)
            done(!w->isError());
    });
}

void Tray::activate(const QString& key) {
    const int row = indexOf(key);
    if (row < 0)
        return;
    const bool menu = !m_items[row].menuPath.isEmpty();
    if (m_items[row].itemIsMenu && menu) {
        emit menuRequested(key);
        return;
    }
    // Screen coordinates mean little on Wayland; apps place windows themselves.
    call(key, u"Activate"_s, {0, 0}, [this, key, menu](bool ok) {
        if (ok) {
            emit activated(key);
        } else if (menu) {
            emit menuRequested(key);
        } else {
            // Not implemented (some apps only have a menu they draw themselves).
            call(key, u"ContextMenu"_s, {0, 0});
            emit activated(key);
        }
    });
}

void Tray::secondaryActivate(const QString& key) {
    call(key, u"SecondaryActivate"_s, {0, 0});
    emit activated(key);
}

void Tray::contextMenu(const QString& key) {
    const int row = indexOf(key);
    if (row < 0)
        return;
    if (!m_items[row].menuPath.isEmpty()) {
        emit menuRequested(key);
        return;
    }
    call(key, u"ContextMenu"_s, {0, 0});
    emit activated(key);
}

QVariantMap Tray::item(const QString& key) const {
    const int row = indexOf(key);
    if (row < 0)
        return {};
    const auto& it = m_items[row];
    return {{u"title"_s, it.title}, {u"icon"_s, it.icon}, {u"service"_s, it.service}, {u"menuPath"_s, it.menuPath}};
}

void Tray::scroll(const QString& key, int delta, bool horizontal) {
    call(key, u"Scroll"_s, {delta, horizontal ? u"horizontal"_s : u"vertical"_s});
}

} // namespace island

#include "tray.moc"
