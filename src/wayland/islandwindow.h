#pragma once

#include <QQmlParserStatus>
#include <QQuickWindow>
#include <QRectF>
#include <QtQml/qqmlregistration.h>

namespace LayerShellQt {
class Window;
}

namespace island {

// One overlay layer surface per monitor, anchored to the top edge.
//
// The surface size and exclusive zone are set once and never change: the
// island animates inside the surface, so Hyprland never re-lays out windows.
// `inputRect` is the only thing that follows the pill; it becomes the Wayland
// input region so clicks outside the pill reach the windows below.
class IslandWindow : public QQuickWindow, public QQmlParserStatus {
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT

    // Output name (e.g. "DP-2"). QML's Qt.application.screens hands out
    // QQuickScreenInfo, not QScreen, so screens are matched by name.
    Q_PROPERTY(QString screenName READ screenName WRITE setScreenName NOTIFY screenNameChanged)
    Q_PROPERTY(QRectF inputRect READ inputRect WRITE setInputRect NOTIFY inputRectChanged)
    Q_PROPERTY(int exclusiveZone READ exclusiveZone WRITE setExclusiveZone NOTIFY
                   exclusiveZoneChanged)

public:
    explicit IslandWindow(QWindow* parent = nullptr);

    QString screenName() const { return m_screenName; }
    void setScreenName(const QString& name);

    QRectF inputRect() const { return m_inputRect; }
    void setInputRect(const QRectF& rect);

    int exclusiveZone() const { return m_exclusiveZone; }
    void setExclusiveZone(int zone);

    void classBegin() override {}
    void componentComplete() override;

signals:
    void screenNameChanged();
    void inputRectChanged();
    void exclusiveZoneChanged();

private:
    void applyMask();

    LayerShellQt::Window* m_layer = nullptr;
    QString m_screenName;
    QRectF m_inputRect;
    int m_exclusiveZone = 44;
    bool m_complete = false;
};

} // namespace island
