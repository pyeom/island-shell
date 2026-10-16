#pragma once

#include <QObject>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlListProperty>
#include <QQmlParserStatus>
#include <QtQml/qqmlregistration.h>

namespace island {

// Something that can occupy the island. Modeled on iOS ActivityKit: an
// activity declares how it looks in each presentation and the island picks
// which one to show.
//
//   Activity {
//       name: "media"; priority: 50
//       active: player.playing
//       compactLeading: Component { AlbumArt {} }
//       compactTrailing: Component { Waveform {} }
//       minimal: Component { AlbumArt {} }
//       expanded: Component { MediaPanel {} }
//   }
//
// Presentations are components because every island (one per monitor)
// instantiates its own copy.
class Activity : public QObject, public QQmlParserStatus {
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(int priority READ priority WRITE setPriority NOTIFY priorityChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(Scope scope READ scope WRITE setScope NOTIFY scopeChanged)
    // Modal activities (launcher, power menu) open expanded, grab the
    // keyboard and stay until they turn inactive.
    Q_PROPERTY(bool modal READ modal WRITE setModal NOTIFY modalChanged)
    // Ambient activities (clock, media, privacy) are background information:
    // the island hides them while a window is fullscreen.
    Q_PROPERTY(bool ambient READ ambient WRITE setAmbient NOTIFY ambientChanged)
    Q_PROPERTY(QQmlComponent* compactLeading READ compactLeading WRITE setCompactLeading NOTIFY
                   presentationsChanged)
    Q_PROPERTY(QQmlComponent* compactTrailing READ compactTrailing WRITE setCompactTrailing NOTIFY
                   presentationsChanged)
    Q_PROPERTY(QQmlComponent* minimal READ minimal WRITE setMinimal NOTIFY presentationsChanged)
    Q_PROPERTY(QQmlComponent* expanded READ expanded WRITE setExpanded NOTIFY presentationsChanged)
    Q_PROPERTY(QQmlListProperty<QObject> data READ data)
    Q_CLASSINFO("DefaultProperty", "data")

public:
    enum Scope {
        FocusedScreen, // only on the monitor with focus (default)
        AllScreens,
    };
    Q_ENUM(Scope)

    explicit Activity(QObject* parent = nullptr);
    ~Activity() override;

    QString name() const { return m_name; }
    void setName(const QString& name);
    int priority() const { return m_priority; }
    void setPriority(int priority);
    bool active() const { return m_active; }
    void setActive(bool active);
    Scope scope() const { return m_scope; }
    void setScope(Scope scope);
    bool modal() const { return m_modal; }
    void setModal(bool modal);
    bool ambient() const { return m_ambient; }
    void setAmbient(bool ambient);

    QQmlComponent* compactLeading() const { return m_compactLeading; }
    void setCompactLeading(QQmlComponent* c);
    QQmlComponent* compactTrailing() const { return m_compactTrailing; }
    void setCompactTrailing(QQmlComponent* c);
    QQmlComponent* minimal() const { return m_minimal; }
    void setMinimal(QQmlComponent* c);
    QQmlComponent* expanded() const { return m_expanded; }
    void setExpanded(QQmlComponent* c);

    QQmlListProperty<QObject> data();

    // Briefly expands the island to show this activity (e.g. a new song).
    // Ignored on monitors where the activity is not eligible.
    Q_INVOKABLE void alert(int durationMs = 4000);

    // The island asks a modal activity to close (click outside, islandctl
    // collapse, another modal opening). Handle with
    // `onDismissRequested: open = false`.
    Q_INVOKABLE void dismiss();

    void classBegin() override {}
    void componentComplete() override;

signals:
    void nameChanged();
    void priorityChanged();
    void activeChanged();
    void scopeChanged();
    void modalChanged();
    void ambientChanged();
    void presentationsChanged();
    void alertRequested(int durationMs);
    void dismissRequested();

private:
    void setComponent(QPointer<QQmlComponent>& slot, QQmlComponent* c);

    QString m_name;
    int m_priority = 0;
    bool m_active = true;
    Scope m_scope = FocusedScreen;
    bool m_modal = false;
    bool m_ambient = false;
    QPointer<QQmlComponent> m_compactLeading;
    QPointer<QQmlComponent> m_compactTrailing;
    QPointer<QQmlComponent> m_minimal;
    QPointer<QQmlComponent> m_expanded;
    QList<QObject*> m_data;
};

} // namespace island
