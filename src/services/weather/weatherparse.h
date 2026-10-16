#pragma once

#include <QByteArrayView>
#include <QDateTime>
#include <QList>
#include <QString>

#include <optional>

namespace island::weather {

struct Place {
    double latitude = 0;
    double longitude = 0;
    QString name;
    QString country;
};

struct Hour {
    QDateTime time; // local time of the place
    double temperature = 0;
    int precipitationProbability = 0; // %
    int code = 0;                     // WMO weather code
};

struct Forecast {
    double temperature = 0;
    int code = 0;
    bool isDay = true;
    QList<Hour> hours;
};

// Open-Meteo geocoding: first result.
std::optional<Place> parseGeocode(QByteArrayView json);
// Open-Meteo forecast with current=temperature_2m,weather_code,is_day and
// hourly=temperature_2m,precipitation_probability,weather_code.
std::optional<Forecast> parseForecast(QByteArrayView json);

// Freedesktop icon name for a WMO code ("weather-showers", ...).
QString iconName(int code, bool isDay);
// Human description key for a WMO code (English; translated by callers).
const char* description(int code);

// First hour within `within` hours after `now` whose precipitation
// probability reaches `threshold`, if any.
std::optional<Hour> rainSoon(const Forecast& f, const QDateTime& now, int threshold, int within = 2);

} // namespace island::weather
