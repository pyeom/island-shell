#include "services/tray/trayparse.h"

#include <QTest>

using namespace island;
using namespace Qt::StringLiterals;

namespace {
// A square pixmap of one ARGB color, in network byte order.
TrayPixmap solid(int size, quint32 argb) {
    TrayPixmap p{size, size, {}};
    for (int i = 0; i < size * size; ++i) {
        for (int shift : {24, 16, 8, 0})
            p.data.append(char((argb >> shift) & 0xff));
    }
    return p;
}
} // namespace

class TestTrayParse : public QObject {
    Q_OBJECT

private slots:
    void address() {
        // A bus name: the default object.
        auto a = parseTrayAddress(u"org.kde.StatusNotifierItem-42-1"_s, u":1.7"_s);
        QCOMPARE(a.service, u"org.kde.StatusNotifierItem-42-1"_s);
        QCOMPARE(a.path, u"/StatusNotifierItem"_s);
        // An object path (libappindicator, Electron): on the caller's connection.
        a = parseTrayAddress(u"/org/ayatana/NotificationItem/discord"_s, u":1.7"_s);
        QCOMPARE(a.service, u":1.7"_s);
        QCOMPARE(a.path, u"/org/ayatana/NotificationItem/discord"_s);
        QCOMPARE(a.key(), u":1.7/org/ayatana/NotificationItem/discord"_s);
        // What a watcher lists: both together.
        a = parseTrayAddress(u":1.7/org/ayatana/NotificationItem/discord"_s);
        QCOMPARE(a.service, u":1.7"_s);
        QCOMPARE(a.path, u"/org/ayatana/NotificationItem/discord"_s);
        // Nothing: the caller, default object.
        a = parseTrayAddress({}, u":1.9"_s);
        QCOMPARE(a.key(), u":1.9/StatusNotifierItem"_s);
    }

    void pixmap() {
        QVERIFY(trayPixmapImage({}, 32).isNull());
        // Short data is skipped.
        QVERIFY(trayPixmapImage({TrayPixmap{16, 16, QByteArray(10, 0)}}, 32).isNull());

        const auto image = trayPixmapImage({solid(16, 0xffff0000), solid(48, 0x8000ff00), solid(64, 0xff0000ff)}, 32);
        QCOMPARE(image.width(), 48); // smallest that is big enough
        QCOMPARE(image.pixel(3, 5), 0x8000ff00u);
        // None big enough: the largest.
        QCOMPARE(trayPixmapImage({solid(16, 0xffff0000), solid(22, 0xffff0000)}, 64).width(), 22);
    }

    void menuLabel() {
        QCOMPARE(trayMenuLabel(u"_Quit"_s), u"Quit"_s);
        QCOMPARE(trayMenuLabel(u"Save __as"_s), u"Save _as"_s);
        QCOMPARE(trayMenuLabel(u"Plain"_s), u"Plain"_s);
        QCOMPARE(trayMenuLabel(u"trailing_"_s), u"trailing"_s);
    }
};

QTEST_GUILESS_MAIN(TestTrayParse)
#include "tst_trayparse.moc"
