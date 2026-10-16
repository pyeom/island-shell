#pragma once

#include "weatherparse.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace island {

// Weather from Open-Meteo (no API key) for [weather] city. The city is
// geocoded once and cached in $XDG_STATE_HOME/island-shell/weather.json; the
// forecast refreshes every 30 minutes.
class Weather : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(QString place READ place NOTIFY changed)
    Q_PROPERTY(qreal temperature READ temperature NOTIFY changed)
    Q_PROPERTY(QString iconName READ iconName NOTIFY changed)
    Q_PROPERTY(QString description READ description NOTIFY changed)
    Q_PROPERTY(QString unit READ unit NOTIFY changed) // "°C" | "°F"
    // Next hours: [{ time, temperature, icon, precipitation }]
    Q_PROPERTY(QVariantList hours READ hours NOTIFY changed)
    // { time, probability } when rain is likely within 2 hours, else {}.
    Q_PROPERTY(QVariantMap rainSoon READ rainSoon NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)

public:
    static Weather* create(QQmlEngine*, QJSEngine*);

    bool available() const { return m_forecast.has_value(); }
    QString place() const { return m_place.name; }
    qreal temperature() const { return m_forecast ? m_forecast->temperature : 0; }
    QString iconName() const;
    QString description() const;
    QString unit() const;
    QVariantList hours() const;
    QVariantMap rainSoon() const;
    QString error() const { return m_error; }

public slots:
    void refresh();

signals:
    void changed();

private:
    explicit Weather(QObject* parent = nullptr);
    void geocode(const QString& city);
    void fetchForecast();
    void fail(const QString& message);
    QString cachePath() const;

    QNetworkAccessManager m_net;
    QTimer m_timer;
    QString m_city;
    weather::Place m_place;
    bool m_havePlace = false;
    std::optional<weather::Forecast> m_forecast;
    QString m_error;
};

} // namespace island
