#include "spring.h"

#include <QQuickItem>
#include <QQuickWindow>

namespace island {

Spring::Spring(QObject* parent) : QObject(parent) {
    m_model.setResponse(m_response, m_dampingFraction);
    m_fallbackTimer.setTimerType(Qt::PreciseTimer);
    m_fallbackTimer.setInterval(16);
    connect(&m_fallbackTimer, &QTimer::timeout, this, &Spring::tick);
}

void Spring::setTarget(qreal target) {
    if (m_initialized && qFuzzyCompare(m_model.target, target))
        return;
    m_model.target = target;
    emit targetChanged();

    // The first target is the resting position: no animation from 0.
    if (!m_initialized || !m_animated) {
        m_initialized = true;
        m_model.snap();
        emit valueChanged();
        stop();
        return;
    }
    start();
}

void Spring::setResponse(qreal response) {
    if (qFuzzyCompare(m_response, response))
        return;
    m_response = response;
    m_model.setResponse(m_response, m_dampingFraction);
    emit responseChanged();
}

void Spring::setDampingFraction(qreal fraction) {
    if (qFuzzyCompare(m_dampingFraction, fraction))
        return;
    m_dampingFraction = fraction;
    m_model.setResponse(m_response, m_dampingFraction);
    emit dampingFractionChanged();
}

void Spring::setAnimated(bool animated) {
    if (m_animated == animated)
        return;
    m_animated = animated;
    emit animatedChanged();
    if (!m_animated && m_running) {
        m_model.snap();
        emit valueChanged();
        stop();
    }
}

void Spring::componentComplete() {
    attachToItem();
}

void Spring::attachToItem() {
    for (QObject* p = parent(); p; p = p->parent()) {
        if (auto* item = qobject_cast<QQuickItem*>(p)) {
            m_item = item;
            connect(item, &QQuickItem::windowChanged, this, &Spring::setWindow);
            setWindow(item->window());
            return;
        }
    }
}

void Spring::setWindow(QQuickWindow* window) {
    if (m_window == window)
        return;
    disconnect(m_frameConnection);
    m_window = window;
    if (m_window) {
        m_frameConnection =
            connect(m_window, &QQuickWindow::afterAnimating, this, &Spring::tick);
    }
    if (m_running) {
        m_fallbackTimer.stop();
        requestFrame();
    }
}

void Spring::start() {
    if (!m_clock.isValid())
        m_clock.start();
    if (!m_running) {
        m_lastNs = m_clock.nsecsElapsed();
        m_running = true;
        emit runningChanged();
    }
    requestFrame();
}

void Spring::stop() {
    m_fallbackTimer.stop();
    if (m_running) {
        m_running = false;
        emit runningChanged();
    }
}

void Spring::tick() {
    if (!m_running)
        return;
    const qint64 now = m_clock.nsecsElapsed();
    const double dt = static_cast<double>(now - m_lastNs) / 1e9;
    m_lastNs = now;

    m_model.step(dt);
    if (m_model.settled()) {
        m_model.snap();
        emit valueChanged();
        stop();
        return;
    }
    emit valueChanged();
    requestFrame();
}

void Spring::requestFrame() {
    if (m_window && m_window->isVisible())
        m_window->update();
    else if (!m_fallbackTimer.isActive())
        m_fallbackTimer.start();
}

} // namespace island
