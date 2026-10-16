// Integration: Config::set edits the real file in place and reloads.
#include "config.h"

#include <QFile>
#include <QGuiApplication>
#include <QTemporaryDir>
#include <QTest>

using island::Config;

class TestConfigSet : public QObject {
    Q_OBJECT

    QTemporaryDir tmp;

    QString read() {
        QFile f(tmp.filePath(QStringLiteral("config.toml")));
        return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
    }

private slots:
    void initTestCase() {
        QFile f(tmp.filePath(QStringLiteral("config.toml")));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("# my settings\n[island]\ncompact_width = 190  # keep this comment\n");
        f.close();
        qputenv("ISLAND_SHELL_CONFIG", tmp.filePath(QStringLiteral("config.toml")).toUtf8());
    }

    void setsAndReloads() {
        auto* config = Config::instance();
        const int before = config->revision();
        QVERIFY(config->set(QStringLiteral("island.compact_width"), 230.0)); // QML-style double
        QCOMPARE(config->compactWidth(), 230);
        QVERIFY(config->revision() > before);
        const auto text = read();
        QVERIFY(text.startsWith(QStringLiteral("# my settings\n")));
        QVERIFY(text.contains(QStringLiteral("compact_width = 230  # keep this comment")));
    }

    void addsSectionsAndLists() {
        auto* config = Config::instance();
        QVERIFY(config->set(QStringLiteral("weather.city"), QStringLiteral("Valparaíso")));
        QVERIFY(config->set(QStringLiteral("theme.blur_opacity"), 0.45));
        QVERIFY(config->set(QStringLiteral("calendar.paths"), QStringList{QStringLiteral("~/cal")}));
        QVERIFY(config->errors().isEmpty());
        QCOMPARE(config->get(QStringLiteral("weather.city")).toString(), QStringLiteral("Valparaíso"));
        QCOMPARE(config->data().theme.blurOpacity, 0.45);
        QCOMPARE(config->data().calendar.paths.front(), std::string("~/cal"));
    }

    void rejectsPathsWithoutSection() {
        QVERIFY(!Config::instance()->set(QStringLiteral("nosection"), 1));
    }
};

QTEST_MAIN(TestConfigSet)
#include "tst_configset.moc"
