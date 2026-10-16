// Integration: Network, Bluetooth and Batteries against fake NetworkManager,
// BlueZ and UPower (scripts/testing/fake-system.py) on a private system bus.
// Skipped when dbus-daemon or python-dbus is missing.
#include "batteries.h"
#include "bluetooth.h"
#include "network.h"

#include <QGuiApplication>
#include <QProcess>
#include <QSignalSpy>
#include <QTest>

using namespace island;

class TestFakeHardware : public QObject {
    Q_OBJECT

    QProcess daemon;
    QProcess fake;
    Network* network = nullptr;
    Bluetooth* bluetooth = nullptr;
    Batteries* batteries = nullptr;

    QString fakeLog() { return QString::fromUtf8(fake.readAllStandardOutput()); }

    QVariantMap networkNamed(const QString& ssid) {
        for (const auto& v : network->networks())
            if (v.toMap().value(QStringLiteral("ssid")) == ssid)
                return v.toMap();
        return {};
    }

    QVariantMap deviceNamed(const QString& name) {
        for (const auto& v : bluetooth->devices())
            if (v.toMap().value(QStringLiteral("name")) == name)
                return v.toMap();
        return {};
    }

private slots:
    void initTestCase() {
        if (QProcess::execute(QStringLiteral("python3"), {QStringLiteral("-I"), QStringLiteral("-c"),
                                                          QStringLiteral("import dbus, gi")}) != 0)
            QSKIP("python-dbus / python-gobject not installed");
        daemon.start(QStringLiteral("dbus-daemon"),
                     {QStringLiteral("--config-file=" SOURCE_DIR "/scripts/nested/session.conf"),
                      QStringLiteral("--nofork"), QStringLiteral("--print-address")});
        if (!daemon.waitForStarted(3000) || !daemon.waitForReadyRead(3000))
            QSKIP("dbus-daemon not available");
        const auto address = QString::fromUtf8(daemon.readLine()).trimmed();
        qputenv("DBUS_SYSTEM_BUS_ADDRESS", address.toUtf8());

        auto env = QProcessEnvironment::systemEnvironment();
        env.insert(QStringLiteral("DBUS_SESSION_BUS_ADDRESS"), address);
        fake.setProcessEnvironment(env);
        fake.start(QStringLiteral("python3"),
                   {QStringLiteral("-I"), QStringLiteral(SOURCE_DIR "/scripts/testing/fake-system.py")});
        QVERIFY(fake.waitForReadyRead(5000));
        QVERIFY(fakeLog().contains(u"ready"));

        network = Network::create(nullptr, nullptr);
        bluetooth = Bluetooth::create(nullptr, nullptr);
        batteries = Batteries::create(nullptr, nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(network->connected(), 3000);
        QTRY_VERIFY_WITH_TIMEOUT(bluetooth->available(), 3000);
    }

    void cleanupTestCase() {
        fake.kill();
        daemon.kill();
        fake.waitForFinished();
        daemon.waitForFinished();
    }

    void wifiStatus() {
        QCOMPARE(network->primaryType(), QStringLiteral("wifi"));
        QTRY_COMPARE(network->ssid(), QStringLiteral("CasaWiFi"));
        QVERIFY(network->hasWifi());
    }

    void networkList() {
        network->setWatching(true);
        QTRY_COMPARE_WITH_TIMEOUT(network->networks().size(), 4, 3000);
        const auto first = network->networks().first().toMap();
        QCOMPARE(first.value(QStringLiteral("ssid")).toString(), QStringLiteral("CasaWiFi")); // active first
        QVERIFY(first.value(QStringLiteral("known")).toBool());
        QVERIFY(!networkNamed(QStringLiteral("Cafe")).value(QStringLiteral("secured")).toBool());
        QVERIFY(networkNamed(QStringLiteral("Vecino")).value(QStringLiteral("secured")).toBool());
    }

    void connectOpenNetwork() {
        network->connectTo(QStringLiteral("Cafe"));
        QTRY_COMPARE_WITH_TIMEOUT(network->ssid(), QStringLiteral("Cafe"), 3000);
        QVERIFY(fakeLog().contains(u"AddAndActivateConnection Cafe psk=none"));
    }

    void wrongPasswordIsReported() {
        network->connectTo(QStringLiteral("Vecino"), QStringLiteral("nope"));
        QTRY_VERIFY_WITH_TIMEOUT(!network->lastError().isEmpty(), 3000);
        QCOMPARE(network->ssid(), QStringLiteral("Cafe"));
    }

    void rightPasswordConnects() {
        network->connectTo(QStringLiteral("Vecino"), QStringLiteral("secreto"));
        QTRY_COMPARE_WITH_TIMEOUT(network->ssid(), QStringLiteral("Vecino"), 3000);
        QVERIFY(network->lastError().isEmpty());
        // Saved now: reconnecting uses the profile, not a new one.
        QTRY_VERIFY_WITH_TIMEOUT(networkNamed(QStringLiteral("Vecino")).value(QStringLiteral("known")).toBool(), 3000);
    }

    void knownNetworkUsesSavedProfile() {
        network->connectTo(QStringLiteral("CasaWiFi"));
        QTRY_COMPARE_WITH_TIMEOUT(network->ssid(), QStringLiteral("CasaWiFi"), 3000);
        QVERIFY(fakeLog().contains(u"ActivateConnection CasaWiFi"));
    }

    void disconnect() {
        network->disconnectWifi();
        QTRY_VERIFY_WITH_TIMEOUT(!network->connected(), 3000);
    }

    void bluetoothDevices() {
        QVERIFY(bluetooth->powered());
        QTRY_VERIFY_WITH_TIMEOUT(!deviceNamed(QStringLiteral("WH-1000XM4")).isEmpty(), 3000);
        const auto phones = deviceNamed(QStringLiteral("WH-1000XM4"));
        QVERIFY(phones.value(QStringLiteral("connected")).toBool());
        QTRY_COMPARE_WITH_TIMEOUT(deviceNamed(QStringLiteral("WH-1000XM4")).value(QStringLiteral("battery")).toInt(), 10, 3000);
        QCOMPARE(bluetooth->connectedDevices(), QStringList{QStringLiteral("WH-1000XM4")});
    }

    void discoveryFindsAndPairs() {
        bluetooth->setDiscovering(true);
        QTRY_VERIFY_WITH_TIMEOUT(!deviceNamed(QStringLiteral("Xbox Controller")).isEmpty(), 3000);
        const auto pad = deviceNamed(QStringLiteral("Xbox Controller"));
        QVERIFY(!pad.value(QStringLiteral("paired")).toBool());
        bluetooth->connectDevice(pad.value(QStringLiteral("path")).toString());
        QTRY_VERIFY_WITH_TIMEOUT(deviceNamed(QStringLiteral("Xbox Controller")).value(QStringLiteral("connected")).toBool(), 3000);
        const auto log = fakeLog();
        QVERIFY(log.contains(u"Bluetooth Pair Xbox Controller"));
        QVERIFY(log.indexOf(u"Bluetooth Pair") < log.indexOf(u"Bluetooth Connect Xbox"));
        bluetooth->setDiscovering(false);
    }

    void batteries_() {
        QTRY_VERIFY_WITH_TIMEOUT(batteries->hasLaptopBattery(), 3000);
        QCOMPARE(batteries->laptopPercentage(), 12);
        QVERIFY(!batteries->charging());
        QTRY_COMPARE_WITH_TIMEOUT(batteries->peripherals().size(), 2, 3000);
        // Laptop battery low and discharging wins the alert.
        QCOMPARE(batteries->low().value(QStringLiteral("percentage")).toInt(), 12);
    }
};

QTEST_MAIN(TestFakeHardware)
#include "tst_fakehardware.moc"
