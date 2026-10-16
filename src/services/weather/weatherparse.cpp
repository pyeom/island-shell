#include "weatherparse.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace island::weather {

std::optional<Place> parseGeocode(QByteArrayView json) {
    const auto results = QJsonDocument::fromJson(json.toByteArray())[QLatin1String("results")].toArray();
    if (results.isEmpty())
        return std::nullopt;
    const auto r = results.first().toObject();
    if (!r.contains(QLatin1String("latitude")))
        return std::nullopt;
    return Place{r[u"latitude"].toDouble(), r[u"longitude"].toDouble(), r[u"name"].toString(),
                 r[u"country"].toString()};
}

std::optional<Forecast> parseForecast(QByteArrayView json) {
    const auto doc = QJsonDocument::fromJson(json.toByteArray());
    const auto current = doc[QLatin1String("current")].toObject();
    if (!current.contains(QLatin1String("temperature_2m")))
        return std::nullopt;
    Forecast f;
    f.temperature = current[u"temperature_2m"].toDouble();
    f.code = current[u"weather_code"].toInt();
    f.isDay = current[u"is_day"].toInt(1) == 1;

    const auto hourly = doc[QLatin1String("hourly")].toObject();
    const auto times = hourly[u"time"].toArray();
    const auto temps = hourly[u"temperature_2m"].toArray();
    const auto precip = hourly[u"precipitation_probability"].toArray();
    const auto codes = hourly[u"weather_code"].toArray();
    for (qsizetype i = 0; i < times.size(); ++i) {
        // "2026-10-15T17:00": local to the place (timezone=auto).
        f.hours.append({QDateTime::fromString(times[i].toString(), Qt::ISODate),
                        temps[i].toDouble(), precip[i].toInt(), codes[i].toInt()});
    }
    return f;
}

QString iconName(int code, bool isDay) {
    const QString night = isDay ? QString() : QStringLiteral("-night");
    if (code == 0)
        return QStringLiteral("weather-clear") + night;
    if (code <= 2)
        return QStringLiteral("weather-few-clouds") + night;
    if (code == 3)
        return QStringLiteral("weather-overcast");
    if (code == 45 || code == 48)
        return QStringLiteral("weather-fog");
    if ((code >= 51 && code <= 57) || code == 80)
        return QStringLiteral("weather-showers-scattered");
    if ((code >= 61 && code <= 67) || code == 81 || code == 82)
        return QStringLiteral("weather-showers");
    if ((code >= 71 && code <= 77) || code == 85 || code == 86)
        return QStringLiteral("weather-snow");
    if (code >= 95)
        return QStringLiteral("weather-storm");
    return QStringLiteral("weather-overcast");
}

const char* description(int code) {
    if (code == 0)
        return "Clear";
    if (code <= 2)
        return "Partly cloudy";
    if (code == 3)
        return "Overcast";
    if (code == 45 || code == 48)
        return "Fog";
    if (code >= 51 && code <= 57)
        return "Drizzle";
    if ((code >= 61 && code <= 67) || (code >= 80 && code <= 82))
        return "Rain";
    if ((code >= 71 && code <= 77) || code == 85 || code == 86)
        return "Snow";
    if (code >= 95)
        return "Thunderstorm";
    return "Cloudy";
}

std::optional<Hour> rainSoon(const Forecast& f, const QDateTime& now, int threshold, int within) {
    if (threshold <= 0)
        return std::nullopt;
    const auto start = now.addSecs(-3600); // the hour in progress counts
    const auto end = now.addSecs(within * 3600LL);
    for (const auto& h : f.hours)
        if (h.time > start && h.time <= end && h.precipitationProbability >= threshold)
            return h;
    return std::nullopt;
}

} // namespace island::weather
