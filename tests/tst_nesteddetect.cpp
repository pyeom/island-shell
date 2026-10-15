#include "services/power/nesteddetect.h"

#include <QTest>

using island::environIsNested;

class TestNestedDetect : public QObject {
    Q_OBJECT

private slots:
    void realCompositorHasNoWaylandDisplay() {
        const QByteArray env("HOME=/home/u\0XDG_RUNTIME_DIR=/run/user/1000\0", 44);
        QVERIFY(!environIsNested(env));
    }

    void nestedCompositorInheritsWaylandDisplay() {
        const QByteArray env("HOME=/home/u\0WAYLAND_DISPLAY=wayland-1\0TERM=x\0", 46);
        QVERIFY(environIsNested(env));
    }

    void emptyValueDoesNotCount() {
        const QByteArray env("WAYLAND_DISPLAY=\0", 17);
        QVERIFY(!environIsNested(env));
    }

    void similarNamesDoNotCount() {
        const QByteArray env("XWAYLAND_DISPLAY=:1\0MY_WAYLAND_DISPLAY=x\0", 40);
        QVERIFY(!environIsNested(env));
    }
};

QTEST_APPLESS_MAIN(TestNestedDetect)
#include "tst_nesteddetect.moc"
