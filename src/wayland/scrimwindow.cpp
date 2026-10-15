#include "scrimwindow.h"

#include <LayerShellQt/Window>
#include <QGuiApplication>
#include <QScreen>

#include <algorithm>

namespace island {

ScrimWindow::ScrimWindow(QWindow* parent) : QQuickWindow(parent) {
    setColor(Qt::transparent);
    setFlag(Qt::FramelessWindowHint);
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"))) {
        m_layer = LayerShellQt::Window::get(this);
        m_layer->setLayer(LayerShellQt::Window::LayerTop);
        m_layer->setAnchors(LayerShellQt::Window::Anchors(
            LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorBottom |
            LayerShellQt::Window::AnchorLeft | LayerShellQt::Window::AnchorRight));
        // -1: cover the whole output, ignoring other surfaces' zones.
        m_layer->setExclusiveZone(-1);
        m_layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
        m_layer->setScope(QStringLiteral("island-shell-scrim"));
        m_layer->setWantsToBeOnActiveScreen(false);
    }
}

void ScrimWindow::setScreenName(const QString& name) {
    if (m_screenName == name)
        return;
    m_screenName = name;
    const auto screens = QGuiApplication::screens();
    const auto it = std::ranges::find(screens, name, &QScreen::name);
    if (it != screens.end()) {
        setScreen(*it);
        if (m_layer)
            m_layer->setScreen(*it);
    }
    emit screenNameChanged();
}

} // namespace island
