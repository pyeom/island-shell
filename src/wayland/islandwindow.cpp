#include "islandwindow.h"

#include <LayerShellQt/Window>
#include <QGuiApplication>
#include <QRegion>
#include <QScreen>

#include <algorithm>

namespace island {

namespace {
bool isWayland() {
    return QGuiApplication::platformName().startsWith(QLatin1String("wayland"));
}
} // namespace

IslandWindow::IslandWindow(QWindow* parent) : QQuickWindow(parent) {
    setColor(Qt::transparent);
    setFlag(Qt::FramelessWindowHint);

    // Layer-shell role must be assigned before the platform window exists.
    if (isWayland()) {
        m_layer = LayerShellQt::Window::get(this);
        m_layer->setLayer(LayerShellQt::Window::LayerOverlay);
        m_layer->setAnchors(LayerShellQt::Window::AnchorTop);
        m_layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
        m_layer->setScope(QStringLiteral("island-shell"));
        m_layer->setWantsToBeOnActiveScreen(false);
        m_layer->setExclusiveZone(m_exclusiveZone);
    }
}

void IslandWindow::setScreenName(const QString& name) {
    if (m_screenName == name)
        return;
    m_screenName = name;
    const auto screens = QGuiApplication::screens();
    const auto it = std::ranges::find(screens, name, &QScreen::name);
    if (it != screens.end()) {
        setScreen(*it);
        if (m_layer)
            m_layer->setScreen(*it);
    } else {
        qWarning("island-shell: no screen named %s", qPrintable(name));
    }
    emit screenNameChanged();
}

void IslandWindow::setInputRect(const QRectF& rect) {
    if (m_inputRect == rect)
        return;
    m_inputRect = rect;
    applyMask();
    emit inputRectChanged();
}

void IslandWindow::setExclusiveZone(int zone) {
    if (m_exclusiveZone == zone)
        return;
    m_exclusiveZone = zone;
    if (m_layer)
        m_layer->setExclusiveZone(zone);
    emit exclusiveZoneChanged();
}

void IslandWindow::componentComplete() {
    m_complete = true;
    applyMask();
    show();
}

void IslandWindow::applyMask() {
    if (!m_complete)
        return;
    // An empty QRegion means "no mask" (the whole surface takes input), which
    // would block the windows below. Keep at least one pixel.
    QRect r = m_inputRect.toAlignedRect();
    if (r.isEmpty())
        r = QRect(0, 0, 1, 1);
    setMask(QRegion(r));
}

} // namespace island
