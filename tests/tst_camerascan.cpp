#include "services/privacy/camerascan.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace island;
using namespace Qt::StringLiterals;

class TestCameraScan : public QObject {
    Q_OBJECT

    // A fake /proc entry: comm and descriptors pointing at `targets`.
    static void process(const QTemporaryDir& proc, const QString& pid, const QString& comm,
                        const QStringList& targets) {
        QDir(proc.path()).mkpath(pid + QLatin1String("/fd"));
        QFile f(proc.filePath(pid + QLatin1String("/comm")));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(comm.toUtf8() + '\n');
        for (int i = 0; i < targets.size(); ++i)
            QVERIFY(QFile::link(targets[i], proc.filePath(pid + QLatin1String("/fd/") + QString::number(i))));
    }

private slots:
    void findsCameraUsers() {
        QTemporaryDir proc;
        process(proc, u"100"_s, u"firefox"_s, {u"/dev/null"_s, u"/dev/video0"_s});
        process(proc, u"200"_s, u"zoom"_s, {u"/dev/video2"_s});
        process(proc, u"201"_s, u"zoom"_s, {u"/dev/video0"_s});
        process(proc, u"300"_s, u"alacritty"_s, {u"/dev/pts/1"_s, u"/tmp/video0"_s});
        process(proc, u"400"_s, u"island-shell"_s, {u"/dev/video0"_s});
        QDir(proc.path()).mkpath(u"self/fd"_s); // not a pid
        QCOMPARE(cameraUsers(proc.path(), 400), QStringList({u"firefox"_s, u"zoom"_s}));
        QVERIFY(cameraUsers(proc.path() + u"/missing"_s).isEmpty());
    }

    void probesNeedNoScan() {
        CameraActivity a;
        a.opened();
        a.closed();
        QVERIFY(!a.needsScan(false)); // a probe
        QVERIFY(a.needsScan(true));   // someone used it before: did they stop?
        a.opened();
        QVERIFY(a.needsScan(false));  // still open
        a.scanned();
        a.closed();
        QVERIFY(!a.needsScan(false));
        a.overflowed();
        QVERIFY(a.needsScan(false));
        a.scanned();
        QVERIFY(!a.needsScan(false));
    }
};

QTEST_GUILESS_MAIN(TestCameraScan)
#include "tst_camerascan.moc"
