#pragma once

#include <QQmlParserStatus>
#include <QQuickWindow>
#include <QList>
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
// `inputRects` is the only thing that follows the pill; it becomes the Wayland
// input region so clicks outside the pill reach the windows below.
class IslandWindow : public QQuickWindow, public QQmlParserStatus {
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT

    // Output name (e.g. "DP-2"). QML's Qt.application.screens hands out
    // QQuickScreenInfo, not QScreen, so screens are matched by name.
    Q_PROPERTY(QString screenName READ screenName WRITE setScreenName NOTIFY screenNameChanged)
    // Union of these becomes the Wayland input region (pill, bubble, ...).
    Q_PROPERTY(QList<QRectF> inputRects READ inputRects WRITE setInputRects NOTIFY
                   inputRectsChanged)
    // Whether this is the monitor with focus. Set from QML (Hyprland state).
    Q_PROPERTY(bool focused READ focused WRITE setFocused NOTIFY focusedChanged)
    Q_PROPERTY(KeyboardMode keyboardMode READ keyboardMode WRITE setKeyboardMode NOTIFY
                   keyboardModeChanged)
    Q_PROPERTY(int exclusiveZone READ exclusiveZone WRITE setExclusiveZone NOTIFY
                   exclusiveZoneChanged)

public:
    enum KeyboardMode {
        NoKeyboard, // never takes keyboard focus (resting state)
        OnDemand,   // focus when clicked (expanded: reply fields, buttons)
        Exclusive,  // grabs the keyboard (launcher, power menu)
    };
    Q_ENUM(KeyboardMode)

    explicit IslandWindow(QWindow* parent = nullptr);

    QString screenName() const { return m_screenName; }
    void setScreenName(const QString& name);

    QList<QRectF> inputRects() const { return m_inputRects; }
    void setInputRects(const QList<QRectF>& rects);

    bool focused() const { return m_focused; }
    void setFocused(bool focused);

    KeyboardMode keyboardMode() const { return m_keyboardMode; }
    void setKeyboardMode(KeyboardMode mode);

    int exclusiveZone() const { return m_exclusiveZone; }
    void setExclusiveZone(int zone);

    void classBegin() override {}
    void componentComplete() override;

signals:
    void screenNameChanged();
    void inputRectsChanged();
    void focusedChanged();
    void keyboardModeChanged();
    void exclusiveZoneChanged();

private:
    void applyMask();

    LayerShellQt::Window* m_layer = nullptr;
    QString m_screenName;
    QList<QRectF> m_inputRects;
    bool m_focused = false;
    KeyboardMode m_keyboardMode = NoKeyboard;
    int m_exclusiveZone = 44;
    bool m_complete = false;
};

} // namespace island
