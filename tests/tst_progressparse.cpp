#include "services/live/progressparse.h"

#include <QDBusVariant>
#include <QTest>

using namespace island;
using namespace Qt::StringLiterals;

class TestProgressParse : public QObject {
    Q_OBJECT

private slots:
    void launcherEntry() {
        auto u = parseLauncherEntry(u"application://chromium.desktop"_s,
                                    {{u"progress"_s, QVariant::fromValue(QDBusVariant(0.42))},
                                     {u"progress-visible"_s, true},
                                     {u"count"_s, 2}});
        QCOMPARE(u.desktopId, u"chromium.desktop"_s);
        QCOMPARE(*u.progress, 0.42);
        QCOMPARE(*u.progressVisible, true);
        QCOMPARE(*u.count, 2);
        QVERIFY(!u.countVisible);

        u = parseLauncherEntry(u"application://telegram.desktop"_s, {{u"count"_s, 5}, {u"count-visible"_s, true}});
        QVERIFY(!u.progress);
        QVERIFY(!u.progressVisible);

        u = parseLauncherEntry(u"application://x.desktop"_s, {{u"progress"_s, 1.7}});
        QCOMPARE(*u.progress, 1.0);
        QVERIFY(parseLauncherEntry(u"file:///x"_s, {}).desktopId.isEmpty());
    }

    void partialDownloads() {
        QCOMPARE(partialDownloadTarget(u"archlinux.iso.part"_s), u"archlinux.iso"_s);
        QCOMPARE(partialDownloadTarget(u"Unconfirmed 1234.crdownload"_s), u"Unconfirmed 1234"_s);
        QVERIFY(partialDownloadTarget(u"report.pdf"_s).isEmpty());
        QVERIFY(partialDownloadTarget(u".part"_s).isEmpty());
    }

    void bytes() {
        QCOMPARE(formatBytes(0), u"0 KB"_s);
        QCOMPARE(formatBytes(512), u"1 KB"_s);
        QCOMPARE(formatBytes(850'000), u"850 KB"_s);
        QCOMPARE(formatBytes(12'400'000), u"12.4 MB"_s);
        QCOMPARE(formatBytes(1'230'000'000), u"1.2 GB"_s);
    }
};

QTEST_GUILESS_MAIN(TestProgressParse)
#include "tst_progressparse.moc"
