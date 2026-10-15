#include "islandscreen.h"

#include "islandwindow.h"

#include <QQuickItem>

namespace island {

IslandScreenAttached::IslandScreenAttached(QObject* attachee) : QObject(attachee) {
    if (auto* item = qobject_cast<QQuickItem*>(attachee)) {
        connect(item, &QQuickItem::windowChanged, this, &IslandScreenAttached::setWindow);
        setWindow(item->window());
    }
}

void IslandScreenAttached::setWindow(QQuickWindow* window) {
    auto* islandWindow = qobject_cast<IslandWindow*>(window);
    if (m_window == islandWindow)
        return;
    if (m_window)
        disconnect(m_window, nullptr, this, nullptr);
    m_window = islandWindow;
    if (m_window) {
        connect(m_window, &IslandWindow::screenNameChanged, this, &IslandScreenAttached::changed);
        connect(m_window, &IslandWindow::focusedChanged, this, &IslandScreenAttached::changed);
    }
    emit changed();
}

QString IslandScreenAttached::name() const {
    return m_window ? m_window->screenName() : QString();
}

bool IslandScreenAttached::focused() const {
    return m_window && m_window->focused();
}

} // namespace island
