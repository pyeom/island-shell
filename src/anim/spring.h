#pragma once

#include "springmodel.h"

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QQmlParserStatus>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class QQuickItem;
class QQuickWindow;

namespace island {

// Animates `value` towards `target` with spring physics.
//
// Ticks are driven by the render loop of the window that owns the parent item
// (QQuickWindow::afterAnimating), so each island steps at its own monitor's
// refresh rate. Without a window it falls back to a ~60 Hz timer.
//
//   Spring { id: w; target: expanded ? 420 : 190 }
//   Rectangle { width: w.value }
class Spring : public QObject, public QQmlParserStatus {
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT

    Q_PROPERTY(qreal target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(qreal value READ value NOTIFY valueChanged)
    Q_PROPERTY(qreal response READ response WRITE setResponse NOTIFY responseChanged)
    Q_PROPERTY(qreal dampingFraction READ dampingFraction WRITE setDampingFraction NOTIFY
                   dampingFractionChanged)
    Q_PROPERTY(bool animated READ animated WRITE setAnimated NOTIFY animatedChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

public:
    explicit Spring(QObject* parent = nullptr);

    qreal target() const { return m_model.target; }
    void setTarget(qreal target);

    qreal value() const { return m_model.value; }

    qreal response() const { return m_response; }
    void setResponse(qreal response);

    qreal dampingFraction() const { return m_dampingFraction; }
    void setDampingFraction(qreal fraction);

    bool animated() const { return m_animated; }
    void setAnimated(bool animated);

    bool running() const { return m_running; }

    void classBegin() override {}
    void componentComplete() override;

signals:
    void targetChanged();
    void valueChanged();
    void responseChanged();
    void dampingFractionChanged();
    void animatedChanged();
    void runningChanged();

private:
    void attachToItem();
    void setWindow(QQuickWindow* window);
    void start();
    void stop();
    void tick();
    void requestFrame();

    SpringModel m_model;
    qreal m_response = 0.45;
    qreal m_dampingFraction = 0.8;
    bool m_animated = true;
    bool m_running = false;
    bool m_initialized = false;

    QPointer<QQuickItem> m_item;
    QPointer<QQuickWindow> m_window;
    QMetaObject::Connection m_frameConnection;
    QTimer m_fallbackTimer;
    QElapsedTimer m_clock;
    qint64 m_lastNs = 0;
};

} // namespace island
