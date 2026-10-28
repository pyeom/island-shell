// Integration: Tray and TrayMenu against fake tray items
// (scripts/testing/faketray.py) on a private session bus. Skipped when
// dbus-daemon or python-dbus is missing.
#include "tray.h"
#include "traymenu.h"

#include <QGuiApplication>
#include <QProcess>
#include <QSignalSpy>
#include <QTest>

using namespace island;
using namespace Qt::StringLiterals;

class TestFakeTray : public QObject {
    Q_OBJECT

    QProcess daemon;
    QProcess fake;
    QString log;
    Tray* tray = nullptr;

    QString fakeLog() {
        log += QString::fromUtf8(fake.readAllStandardOutput());
        return log;
    }

    QString keyOf(const QString& title) {
        for (int row = 0; row < tray->rowCount(); ++row) {
            const auto index = tray->index(row);
            if (tray->data(index, Tray::TitleRole) == title)
                return tray->data(index, Tray::KeyRole).toString();
        }
        return {};
    }

    QVariant role(const QString& key, int role) {
        for (int row = 0; row < tray->rowCount(); ++row) {
            const auto index = tray->index(row);
            if (tray->data(index, Tray::KeyRole) == key)
                return tray->data(index, role);
        }
        return {};
    }

private slots:
    void initTestCase() {
        if (QProcess::execute(u"python3"_s, {u"-I"_s, u"-c"_s, u"import dbus, gi"_s}) != 0)
            QSKIP("python-dbus / python-gobject not installed");
        daemon.start(u"dbus-daemon"_s, {u"--config-file=" SOURCE_DIR "/scripts/nested/session.conf"_s,
                                        u"--nofork"_s, u"--print-address"_s});
        if (!daemon.waitForStarted(3000) || !daemon.waitForReadyRead(3000))
            QSKIP("dbus-daemon not available");
        const auto address = QString::fromUtf8(daemon.readLine()).trimmed();
        // Before anything touches the session bus.
        qputenv("DBUS_SESSION_BUS_ADDRESS", address.toUtf8());

        tray = Tray::instance();
        tray->start();
        QVERIFY(tray->available());

        auto env = QProcessEnvironment::systemEnvironment();
        env.insert(u"DBUS_SESSION_BUS_ADDRESS"_s, address);
        fake.setProcessEnvironment(env);
        fake.start(u"python3"_s, {u"-I"_s, u"" SOURCE_DIR "/scripts/testing/faketray.py"_s, u"30"_s});
        QVERIFY(fake.waitForStarted(3000));
        QTRY_VERIFY_WITH_TIMEOUT(fakeLog().contains(u"registered"), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(tray->count(), 2, 3000);
    }

    void cleanupTestCase() {
        fake.kill();
        daemon.kill();
        fake.waitForFinished();
        daemon.waitForFinished();
    }

    void items() {
        const auto chat = keyOf(u"Fake Chat"_s);
        QVERIFY(chat.endsWith(u"/org/ayatana/NotificationItem/fake_chat"_s)); // registered by path
        QVERIFY(role(chat, Tray::IconRole).toString().startsWith(u"image://tray/"_s)); // from pixels
        QCOMPARE(role(chat, Tray::TooltipRole).toString(), u"3 unread"_s); // HTML stripped
        QVERIFY(role(chat, Tray::HasMenuRole).toBool());

        const auto sync = keyOf(u"Fake Sync"_s);
        QVERIFY(sync.startsWith(u"org.kde.StatusNotifierItem-"_s)); // registered by bus name
        QVERIFY(!role(sync, Tray::HasMenuRole).toBool()); // /NO_DBUSMENU
        QVERIFY(!role(sync, Tray::IconRole).toString().isEmpty());
    }

    void clicks() {
        QSignalSpy activated(tray, &Tray::activated);
        QSignalSpy menu(tray, &Tray::menuRequested);
        const auto chat = keyOf(u"Fake Chat"_s);
        const auto sync = keyOf(u"Fake Sync"_s);

        tray->activate(chat);
        QTRY_COMPARE(activated.count(), 1);
        QTRY_VERIFY(fakeLog().contains(u"fake-chat Activate"));

        // No Activate and no menu: the app's own context menu.
        tray->activate(sync);
        QTRY_COMPARE(activated.count(), 2);
        QTRY_VERIFY(fakeLog().contains(u"fake-sync ContextMenu"));

        tray->contextMenu(chat);
        QCOMPARE(menu.count(), 1);
        tray->scroll(chat, 120);
        QTRY_VERIFY(fakeLog().contains(u"fake-chat Scroll 120 vertical"));
    }

    void menu() {
        const auto chat = keyOf(u"Fake Chat"_s);
        const auto item = tray->item(chat);
        TrayMenu menu;
        menu.setService(item.value(u"service"_s).toString());
        menu.setPath(item.value(u"menuPath"_s).toString());
        QTRY_COMPARE(menu.entries().size(), 6); // "Hidden" left out
        QVERIFY(!menu.loading());

        const auto entry = [&](int i) { return menu.entries().at(i).toMap(); };
        QCOMPARE(entry(0).value(u"label"_s), u"Open Fake Chat"_s);
        QCOMPARE(entry(1).value(u"toggle"_s), u"checkmark"_s);
        QVERIFY(!entry(1).value(u"checked"_s).toBool());
        QVERIFY(entry(2).value(u"separator"_s).toBool());
        QVERIFY(entry(3).value(u"submenu"_s).toBool());
        const auto status = entry(3).value(u"children"_s).toList();
        QCOMPARE(status.size(), 2);
        QCOMPARE(status[0].toMap().value(u"toggle"_s), u"radio"_s);
        QVERIFY(status[0].toMap().value(u"checked"_s).toBool());
        QVERIFY(!entry(4).value(u"enabled"_s).toBool());
        QCOMPARE(entry(5).value(u"label"_s), u"Quit"_s);

        // A click: the app updates its layout and the menu follows.
        QSignalSpy changed(&menu, &TrayMenu::entriesChanged);
        menu.activate(entry(1).value(u"id"_s).toInt());
        QTRY_VERIFY(fakeLog().contains(u"menu Event 2 clicked"));
        QTRY_VERIFY(entry(1).value(u"checked"_s).toBool());
    }

    void appQuits() {
        fake.kill();
        fake.waitForFinished();
        QTRY_COMPARE_WITH_TIMEOUT(tray->rowCount(), 0, 3000);
        QCOMPARE(tray->count(), 0);
    }
};

QTEST_MAIN(TestFakeTray)
#include "tst_faketray.moc"
