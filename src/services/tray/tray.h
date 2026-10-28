#pragma once

#include <QAbstractListModel>
#include <QDBusContext>
#include <QHash>
#include <QImage>
#include <QMutex>
#include <QPointer>
#include <QQuickImageProvider>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <functional>

class QQmlEngine;
class QJSEngine;

namespace island {

class DBusProperties;
class StatusNotifierWatcher;

// image://tray/<n>: icons apps send as pixels (IconPixmap, menu icon-data).
class TrayImageProvider : public QQuickImageProvider {
public:
    TrayImageProvider();
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

    // Keeps `image` and returns its source; `previous` (an earlier source)
    // is dropped.
    static QString store(const QImage& image, const QString& previous = {});
    static void drop(const QString& source);

private:
    static QMutex s_mutex;
    static QHash<QString, QImage> s_images;
    static quint64 s_next;
};

// The system tray: StatusNotifierItems (Discord, Steam, nm-applet...). The
// island serves org.kde.StatusNotifierWatcher itself unless another program
// does, and then reads that one. Unread counts come from the Unity
// LauncherEntry signal (Electron apps never set NeedsAttention).
//
//   Repeater { model: Tray; delegate: ... title, icon, status, hasMenu ... }
class Tray : public QAbstractListModel, protected QDBusContext {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // Watching tray items (the island owns the session bus name).
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    // Items to show: those not Passive.
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    // Of those, the ones that want attention.
    Q_PROPERTY(int attentionCount READ attentionCount NOTIFY countChanged)

public:
    enum Role {
        KeyRole = Qt::UserRole + 1,
        ItemIdRole,
        TitleRole,
        TooltipRole,
        StatusRole, // "Active" | "Passive" | "NeedsAttention"
        ShownRole,  // loaded and not Passive
        BadgeRole,  // unread count the app reports; 0: none
        AttentionRole, // NeedsAttention, or a badge
        IconRole,
        HasMenuRole,
        ItemIsMenuRole,
        ServiceRole,
        MenuPathRole,
    };

    static Tray* instance();
    static Tray* create(QQmlEngine*, QJSEngine*);

    // Starts watching; only the instance that owns the island's bus name.
    void start();

    bool available() const { return m_available; }
    int count() const;
    int attentionCount() const { return m_attentionCount; }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Primary click: Activate, or the menu when the item is only a menu or
    // does not implement Activate (`menuRequested`, else the app's own
    // ContextMenu).
    Q_INVOKABLE void activate(const QString& key);
    Q_INVOKABLE void secondaryActivate(const QString& key);
    // The app's own context menu (items without a dbusmenu).
    Q_INVOKABLE void contextMenu(const QString& key);
    Q_INVOKABLE void scroll(const QString& key, int delta, bool horizontal = false);
    // { title, icon, service, menuPath } of one item; empty when it is gone.
    Q_INVOKABLE QVariantMap item(const QString& key) const;
    // Opens the tray panel on an item's menu (`openRequested`).
    Q_INVOKABLE void showMenu(const QString& key) { emit openRequested(key); }

signals:
    void availableChanged();
    void countChanged();
    void activated(const QString& key);
    void menuRequested(const QString& key);
    void openRequested(const QString& key);

private:
    struct Item {
        QString key;
        QString service;
        QString path;
        QPointer<DBusProperties> props;
        QString itemId;
        QString title;
        QString tooltip;
        QString status = QStringLiteral("Active");
        QString icon;
        QString pixmapSource; // image://tray/ source owned by this item
        QString menuPath;
        QString desktopId; // the installed app, when it can be told
        uint pid = 0;      // of the item's bus connection
        int badge = 0;
        bool itemIsMenu = false;
        bool ready = false; // first properties arrived

        bool shown() const { return ready && (status != u"Passive" || badge > 0); }
        bool attention() const { return status == u"NeedsAttention" || badge > 0; }
    };

    explicit Tray(QObject* parent = nullptr);
    int indexOf(const QString& key) const;
    void setAvailable(bool available);
    void recount();
    // `pid` owns exactly one item (its unread count can only be that one's).
    bool soleItemOf(uint pid) const;
    void useOwnWatcher();
    void useWatcher(const QString& owner);
    void reset();
    void addItem(const QString& registered);
    void removeItem(const QString& registered);
    void update(const QString& key);
    void call(const QString& key, const QString& method, const QVariantList& args,
              const std::function<void(bool ok)>& done = {});
    void refreshDirty();

private slots:
    // org.kde.StatusNotifierWatcher of another program.
    void onItemRegistered(const QString& service);
    void onItemUnregistered(const QString& service);
    // com.canonical.Unity.LauncherEntry.Update
    void onLauncherEntry(const QString& appUri, const QVariantMap& props);

private:
    QList<Item> m_items;
    QStringList m_dirty; // items that sent New* signals
    // Last unread counts, for items that show up after them.
    QHash<QString, int> m_badges;   // by desktop file ID
    QHash<uint, int> m_pidBadges;   // by process
    QTimer m_refresh;
    StatusNotifierWatcher* m_watcher = nullptr;
    QString m_hostName;
    bool m_started = false;
    bool m_available = false;
    int m_count = 0;
    int m_attentionCount = 0;
};

} // namespace island
