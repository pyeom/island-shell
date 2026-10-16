#include "services/weather/weatherparse.h"

#include <QTest>

using namespace island::weather;

class TestWeatherParse : public QObject {
    Q_OBJECT

private slots:
    void geocode() {
        const auto p = parseGeocode(R"({"results":[{"name":"Santiago","latitude":-33.45694,
            "longitude":-70.64827,"country":"Chile"}]})");
        QVERIFY(p);
        QCOMPARE(p->name, QStringLiteral("Santiago"));
        QCOMPARE(p->country, QStringLiteral("Chile"));
        QVERIFY(std::abs(p->latitude + 33.45694) < 1e-6);
        QVERIFY(!parseGeocode(R"({"generationtime_ms":0.5})")); // no results key
    }

    void forecast() {
        const auto f = parseForecast(R"({
            "current":{"time":"2026-10-15T16:00","temperature_2m":18.4,"weather_code":3,"is_day":1},
            "hourly":{"time":["2026-10-15T16:00","2026-10-15T17:00","2026-10-15T18:00"],
                      "temperature_2m":[18.4,17.9,16.2],
                      "precipitation_probability":[10,40,75],
                      "weather_code":[3,61,63]}})");
        QVERIFY(f);
        QCOMPARE(f->temperature, 18.4);
        QCOMPARE(f->code, 3);
        QVERIFY(f->isDay);
        QCOMPARE(f->hours.size(), 3);
        QCOMPARE(f->hours[2].precipitationProbability, 75);
        QVERIFY(!parseForecast("{}"));
    }

    void rain() {
        Forecast f;
        const QDateTime base(QDate(2026, 10, 15), QTime(16, 0));
        f.hours = {{base, 18, 10, 3}, {base.addSecs(3600), 17, 40, 61}, {base.addSecs(7200), 16, 75, 63},
                   {base.addSecs(4 * 3600), 15, 90, 63}};
        const auto now = base.addSecs(15 * 60); // 16:15
        auto r = rainSoon(f, now, 60);
        QVERIFY(r);
        QCOMPARE(r->time, base.addSecs(7200)); // 18:00, within 2 h
        QVERIFY(!rainSoon(f, now, 95));         // nothing that likely
        QVERIFY(!rainSoon(f, now, 0));          // disabled
        QVERIFY(!rainSoon(f, base.addSecs(-5 * 3600), 60)); // too far ahead
    }

    void icons() {
        QCOMPARE(iconName(0, true), QStringLiteral("weather-clear"));
        QCOMPARE(iconName(0, false), QStringLiteral("weather-clear-night"));
        QCOMPARE(iconName(63, true), QStringLiteral("weather-showers"));
        QCOMPARE(iconName(95, true), QStringLiteral("weather-storm"));
        QCOMPARE(QString::fromLatin1(description(73)), QStringLiteral("Snow"));
    }
};

QTEST_APPLESS_MAIN(TestWeatherParse)
#include "tst_weatherparse.moc"
