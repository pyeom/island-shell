#pragma once

#include <QAbstractListModel>
#include <QDBusContext>
#include <QDBusServiceWatcher>
#include <QDateTime>
#include <QTimer>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// Live activities pushed by other programs over D-Bus or `islandctl push`.
// QML instantiates one Activity per row. See
// data/dbus/org.islandshell.LiveActivities.xml for the props.
class LiveActivities : public QAbstractListModel, protected QDBusContext {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    enum Role {
        LiveIdRole = Qt::UserRole + 1,
        TitleRole,
        SubtitleRole,
        IconRole,
        ProgressRole, // -1: none
        StateRole,
        ActionsRole, // [{ key, label }]
        AlertSerialRole, // bumps when the island should briefly expand
    };

    static LiveActivities* instance();
    static LiveActivities* create(QQmlEngine*, QJSEngine*);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // From QML: an action button was clicked.
    Q_INVOKABLE void invoke(const QString& id, const QString& key);

    void registerOnBus();

public slots:
    // org.islandshell.LiveActivities
    void Push(const QString& id, const QVariantMap& props);
    void Update(const QString& id, const QVariantMap& props);
    void End(const QString& id);
    QStringList List();

signals:
    void ActionInvoked(const QString& id, const QString& action);

private:
    struct Item {
        QString id;
        QString title;
        QString subtitle;
        QString icon;
        double progress = -1;
        QString state = QStringLiteral("running");
        QStringList actions; // "key=Label"
        QVariantMap commands; // key -> shell command
        QString owner;        // unique bus name; empty when persistent
        QDateTime expires;    // invalid: never
        int alertSerial = 0;
    };

    explicit LiveActivities(QObject* parent = nullptr);
    int indexOf(const QString& id) const;
    bool apply(Item& item, const QVariantMap& props, QString* error);
    void endIndex(int row);
    void checkExpired();
    void onOwnerGone(const QString& service);
    void fail(const QString& message);

    QList<Item> m_items;
    QDBusServiceWatcher m_watcher;
    QTimer m_ttlTimer;
};

} // namespace island
