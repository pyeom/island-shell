#pragma once

#include <QQmlParserStatus>
#include <QQuickWindow>
#include <QtQml/qqmlregistration.h>

namespace LayerShellQt {
class Window;
}

namespace island {

// Full-screen transparent layer (top layer, below the island) shown while a
// modal activity is open. Clicking it dismisses the modal, the way tapping
// outside closes iOS's expanded island.
class ScrimWindow : public QQuickWindow, public QQmlParserStatus {
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT

    Q_PROPERTY(QString screenName READ screenName WRITE setScreenName NOTIFY screenNameChanged)

public:
    explicit ScrimWindow(QWindow* parent = nullptr);

    QString screenName() const { return m_screenName; }
    void setScreenName(const QString& name);

    void classBegin() override {}
    void componentComplete() override {}

signals:
    void screenNameChanged();

private:
    LayerShellQt::Window* m_layer = nullptr;
    QString m_screenName;
};

} // namespace island
