#include "islandwindow.h"

#include <LayerShellQt/Window>
#include <QGuiApplication>
#include <QRegion>
#include <QScreen>
#include <qpa/qplatformnativeinterface.h>

#include <wayland-client.h>

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

void IslandWindow::setInputRects(const QList<QRectF>& rects) {
    if (m_inputRects == rects)
        return;
    m_inputRects = rects;
    applyMask();
    emit inputRectsChanged();
}

void IslandWindow::setFocused(bool focused) {
    if (m_focused == focused)
        return;
    m_focused = focused;
    emit focusedChanged();
}

void IslandWindow::setExclusiveZone(int zone) {
    if (m_exclusiveZone == zone)
        return;
    m_exclusiveZone = zone;
    if (m_layer)
        m_layer->setExclusiveZone(zone);
    emit exclusiveZoneChanged();
}

void IslandWindow::setKeyboardMode(KeyboardMode mode) {
    if (m_keyboardMode == mode)
        return;
    m_keyboardMode = mode;
    if (m_layer) {
        m_layer->setKeyboardInteractivity(
            mode == Exclusive  ? LayerShellQt::Window::KeyboardInteractivityExclusive
            : mode == OnDemand ? LayerShellQt::Window::KeyboardInteractivityOnDemand
                               : LayerShellQt::Window::KeyboardInteractivityNone);
        // The new interactivity applies on the next commit.
        update();
    }
    if (mode == Exclusive)
        requestActivate();
    emit keyboardModeChanged();
}

void IslandWindow::componentComplete() {
    m_complete = true;
    applyMask();
    show();
}

bool IslandWindow::setEmptyInputRegion() {
    auto* app = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
    auto* native = QGuiApplication::platformNativeInterface();
    if (!app || !native || !app->compositor())
        return false;
    auto* surface = static_cast<wl_surface*>(native->nativeResourceForWindow("surface", this));
    if (!surface)
        return false;
    wl_region* region = wl_compositor_create_region(app->compositor());
    wl_surface_set_input_region(surface, region);
    wl_region_destroy(region);
    update(); // the input region is double-buffered: apply on the next commit
    return true;
}

void IslandWindow::applyMask() {
    if (!m_complete)
        return;
    QRegion region;
    for (const auto& r : std::as_const(m_inputRects))
        region += r.toAlignedRect();

    if (region.isEmpty()) {
        // QWindow::setMask(QRegion()) means "no mask": the whole surface
        // would take input and block what is below (a fullscreen game while
        // the island is hidden). Set a truly empty region on the surface.
        if (setEmptyInputRegion()) {
            m_inputEmpty = true;
            return;
        }
        region = QRect(width() - 1, height() - 1, 1, 1); // not on Wayland
    }
    if (m_inputEmpty) {
        // Qt skips setMask() when the region equals the last one it applied;
        // reset it so the region replaces our empty one.
        setMask(QRegion());
        m_inputEmpty = false;
    }
    setMask(region);
}

} // namespace island
