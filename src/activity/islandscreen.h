#pragma once

#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

class QQuickItem;
class QQuickWindow;

namespace island {

class IslandWindow;

class IslandScreenAttached : public QObject {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(QString name READ name NOTIFY changed)
    Q_PROPERTY(bool focused READ focused NOTIFY changed)

public:
    explicit IslandScreenAttached(QObject* attachee);

    QString name() const;
    bool focused() const;

signals:
    void changed();

private:
    void setWindow(QQuickWindow* window);

    QPointer<IslandWindow> m_window;
};

// Which monitor an activity presentation is being drawn on:
//
//   Text { text: IslandScreen.name; visible: IslandScreen.focused }
class IslandScreen : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("IslandScreen is an attached property")
    QML_ATTACHED(IslandScreenAttached)

public:
    static IslandScreenAttached* qmlAttachedProperties(QObject* object) {
        return new IslandScreenAttached(object);
    }
};

} // namespace island
