#pragma once

#include <QAbstractListModel>
#include <QDBusContext>
#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

struct NotificationData {
    uint id = 0;
    QString appName;
    QString appIcon;      // as sent: theme name, path or URL
    QString desktopEntry;
    QString summary;
    QString body;         // sanitized markup
    QString image;        // file path or URL of the attached image
    bool imageCached = false; // `image` is ours to delete
    QStringList actions;  // flat key, label, key, label ...
    int urgency = 1;
    int expireTimeout = -1;
    bool transient = false;
    bool resident = false;
    bool inlineReply = false;
    QString replyPlaceholder;
    QDateTime time;
};

class NotificationModel : public QAbstractListModel {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        AppNameRole,
        IconRole,
        SummaryRole,
        BodyRole,
        ImageRole,
        ActionsRole,
        UrgencyRole,
        TimeRole,
        HasReplyRole,
        ReplyPlaceholderRole,
    };

    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    const QList<NotificationData>& items() const { return m_items; }
    const NotificationData* find(uint id) const;
    // Inserts at the top, or replaces in place and moves it to the top.
    void upsert(const NotificationData& n);
    bool remove(uint id);
    void clear();
    void trim(int limit);

    static QVariantMap toMap(const NotificationData& n);

signals:
    void countChanged();

private:
    int indexOf(uint id) const;
    QList<NotificationData> m_items;
};

// org.freedesktop.Notifications server, popup queue and history.
//
// Popups go one at a time through `current`; the island shows it while it is
// set. History is kept newest first and saved to
// $XDG_STATE_HOME/island-shell/notifications.json.
class Notifications : public QObject, protected QDBusContext {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(QString unavailableReason READ unavailableReason NOTIFY availableChanged)
    Q_PROPERTY(island::NotificationModel* history READ history CONSTANT)
    // The popup being shown, or an empty map: { id, appName, icon, summary,
    // body, image, actions: [{key, label}], urgency, time, hasReply, ... }
    Q_PROPERTY(QVariantMap current READ current NOTIFY currentChanged)
    // While true (pointer over the popup) the popup does not time out.
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)

public:
    static Notifications* instance();
    static Notifications* create(QQmlEngine*, QJSEngine*);

    // Tries to own org.freedesktop.Notifications; never steals it.
    void registerOnBus();

    bool available() const { return m_available; }
    QString unavailableReason() const { return m_unavailableReason; }
    NotificationModel* history() { return &m_history; }
    QVariantMap current() const;
    bool paused() const { return m_paused; }
    void setPaused(bool paused);

    Q_INVOKABLE void invokeAction(uint id, const QString& key);
    Q_INVOKABLE void reply(uint id, const QString& text);
    // Removes it from history and tells the app it was dismissed.
    Q_INVOKABLE void dismiss(uint id);
    // Hides the current popup; it stays in history.
    Q_INVOKABLE void dismissPopup();
    Q_INVOKABLE void clearHistory();

    // org.freedesktop.Notifications; not a slot because out-parameters are
    // not valid QML method arguments. The adaptor calls it directly.
    QString GetServerInformation(QString& vendor, QString& version, QString& spec_version);

public slots:
    // org.freedesktop.Notifications
    QStringList GetCapabilities();
    uint Notify(const QString& app_name, uint replaces_id, const QString& app_icon,
                const QString& summary, const QString& body, const QStringList& actions,
                const QVariantMap& hints, int expire_timeout);
    void CloseNotification(uint id);

    // org.islandshell.Notifications.History
    QList<QVariantMap> List();
    void Clear();
    void Dismiss(uint id);

signals:
    // D-Bus signals.
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString& action_key);
    void NotificationReplied(uint id, const QString& text);

    void availableChanged();
    void currentChanged();
    void pausedChanged();
    // A new popup became current; the island expands for `timeoutMs`
    // (0: until dismissed).
    void popupShown(int timeoutMs);

private:
    enum CloseReason : uint { Expired = 1, Dismissed = 2, ClosedByCall = 3 };

    explicit Notifications(QObject* parent = nullptr);
    void showNext();
    void endPopup();
    int popupTimeout(const NotificationData& n) const;
    void close(uint id, CloseReason reason);
    void removeEverywhere(uint id);
    QString storeImage(uint id, const QVariant& imageData);
    void load();
    void scheduleSave();
    void save();

    NotificationModel m_history;
    QList<uint> m_queue;
    uint m_current = 0;
    uint m_nextId = 1;
    bool m_paused = false;
    bool m_available = false;
    QString m_unavailableReason;
    QTimer m_popupTimer;
    QTimer m_saveTimer;
    QString m_statePath;
    QString m_cacheDir;
};

} // namespace island
