#include "weather.h"

#include "config.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QUrlQuery>

Q_LOGGING_CATEGORY(lcWeather, "island.weather")

using namespace Qt::StringLiterals;

namespace island {

Weather* Weather::create(QQmlEngine*, QJSEngine*) {
    static auto* w = new Weather(QCoreApplication::instance());
    QJSEngine::setObjectOwnership(w, QJSEngine::CppOwnership);
    return w;
}

Weather::Weather(QObject* parent) : QObject(parent) {
    m_timer.setInterval(30 * 60 * 1000);
    connect(&m_timer, &QTimer::timeout, this, &Weather::refresh);
    m_timer.start();
    connect(Config::instance(), &Config::changed, this, [this] {
        if (QString::fromStdString(Config::instance()->data().weather.city) != m_city)
            refresh();
    });
    refresh();
}

QString Weather::cachePath() const {
    return QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation) +
           QLatin1String("/island-shell/weather.json");
}

void Weather::refresh() {
    const auto city = QString::fromStdString(Config::instance()->data().weather.city).trimmed();
    if (city.isEmpty()) {
        m_city.clear();
        m_forecast.reset();
        m_error.clear();
        emit changed();
        return;
    }
    if (city != m_city) {
        m_city = city;
        m_havePlace = false;
        // Cached geocode for this city.
        QFile f(cachePath());
        if (f.open(QIODevice::ReadOnly)) {
            const auto o = QJsonDocument::fromJson(f.readAll()).object();
            if (o[u"city"].toString() == city) {
                m_place = {o[u"latitude"].toDouble(), o[u"longitude"].toDouble(),
                           o[u"name"].toString(), o[u"country"].toString()};
                m_havePlace = true;
            }
        }
    }
    if (m_havePlace)
        fetchForecast();
    else
        geocode(city);
}

void Weather::geocode(const QString& city) {
    QUrl url(QStringLiteral("https://geocoding-api.open-meteo.com/v1/search"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("name"), city);
    q.addQueryItem(QStringLiteral("count"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("language"), QLocale::system().name().section(u'_', 0, 0));
    url.setQuery(q);
    auto* reply = m_net.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, city] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            fail(reply->errorString());
            return;
        }
        const auto place = weather::parseGeocode(reply->readAll());
        if (!place) {
            fail(QStringLiteral("city \"%1\" not found").arg(city));
            return;
        }
        m_place = *place;
        m_havePlace = true;
        QDir().mkpath(QFileInfo(cachePath()).path());
        QFile f(cachePath());
        if (f.open(QIODevice::WriteOnly))
            f.write(QJsonDocument(QJsonObject{{u"city"_s, city},
                                              {u"latitude"_s, place->latitude},
                                              {u"longitude"_s, place->longitude},
                                              {u"name"_s, place->name},
                                              {u"country"_s, place->country}})
                        .toJson(QJsonDocument::Compact));
        fetchForecast();
    });
}

void Weather::fetchForecast() {
    const bool imperial = Config::instance()->data().weather.units == "imperial";
    QUrl url(QStringLiteral("https://api.open-meteo.com/v1/forecast"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("latitude"), QString::number(m_place.latitude, 'f', 4));
    q.addQueryItem(QStringLiteral("longitude"), QString::number(m_place.longitude, 'f', 4));
    q.addQueryItem(QStringLiteral("current"), QStringLiteral("temperature_2m,weather_code,is_day"));
    q.addQueryItem(QStringLiteral("hourly"),
                   QStringLiteral("temperature_2m,precipitation_probability,weather_code"));
    q.addQueryItem(QStringLiteral("forecast_hours"), QStringLiteral("12"));
    q.addQueryItem(QStringLiteral("timezone"), QStringLiteral("auto"));
    if (imperial)
        q.addQueryItem(QStringLiteral("temperature_unit"), QStringLiteral("fahrenheit"));
    url.setQuery(q);
    auto* reply = m_net.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            fail(reply->errorString());
            return;
        }
        auto forecast = weather::parseForecast(reply->readAll());
        if (!forecast) {
            fail(QStringLiteral("unexpected forecast response"));
            return;
        }
        m_forecast = std::move(forecast);
        m_error.clear();
        qCInfo(lcWeather).noquote() << "forecast for" << m_place.name << m_forecast->temperature;
        emit changed();
    });
}

void Weather::fail(const QString& message) {
    m_error = message;
    qCWarning(lcWeather).noquote() << message;
    emit changed();
}

QString Weather::iconName() const {
    return m_forecast ? weather::iconName(m_forecast->code, m_forecast->isDay) : QString();
}

QString Weather::description() const {
    return m_forecast ? QCoreApplication::translate("Weather", weather::description(m_forecast->code))
                      : QString();
}

QString Weather::unit() const {
    return Config::instance()->data().weather.units == "imperial" ? QStringLiteral("°F")
                                                                  : QStringLiteral("°C");
}

QVariantList Weather::hours() const {
    QVariantList out;
    if (!m_forecast)
        return out;
    const auto now = QDateTime::currentDateTime();
    for (const auto& h : m_forecast->hours) {
        if (h.time < now.addSecs(-3600))
            continue;
        if (out.size() >= 6)
            break;
        out.append(QVariantMap{
            {QStringLiteral("time"), h.time},
            {QStringLiteral("temperature"), h.temperature},
            {QStringLiteral("icon"), weather::iconName(h.code, h.time.time().hour() >= 7 && h.time.time().hour() < 20)},
            {QStringLiteral("precipitation"), h.precipitationProbability},
        });
    }
    return out;
}

QVariantMap Weather::rainSoon() const {
    if (!m_forecast)
        return {};
    const auto h = weather::rainSoon(*m_forecast, QDateTime::currentDateTime(),
                                     Config::instance()->data().weather.rainAlert);
    if (!h)
        return {};
    return {{QStringLiteral("time"), h->time}, {QStringLiteral("probability"), h->precipitationProbability}};
}

} // namespace island

// lupdate: weather descriptions from weatherparse.cpp.
[[maybe_unused]] static const char* const kWeatherStrings[] = {
    QT_TRANSLATE_NOOP("Weather", "Clear"),   QT_TRANSLATE_NOOP("Weather", "Partly cloudy"),
    QT_TRANSLATE_NOOP("Weather", "Overcast"), QT_TRANSLATE_NOOP("Weather", "Fog"),
    QT_TRANSLATE_NOOP("Weather", "Drizzle"), QT_TRANSLATE_NOOP("Weather", "Rain"),
    QT_TRANSLATE_NOOP("Weather", "Snow"),    QT_TRANSLATE_NOOP("Weather", "Thunderstorm"),
    QT_TRANSLATE_NOOP("Weather", "Cloudy"),
};
