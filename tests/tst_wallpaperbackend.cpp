#include "services/wallpaper/wallpaperbackend.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace island::wallpaper;

class TestWallpaperBackend : public QObject {
    Q_OBJECT

private slots:
    void names() {
        for (const auto b : {Backend::Awww, Backend::Swww, Backend::Hyprpaper, Backend::Swaybg,
                             Backend::Waypaper, Backend::Command})
            QCOMPARE(backendFromName(backendName(b)), b);
        QVERIFY(!backendFromName(u"auto"));
        QVERIFY(!backendFromName(u"feh"));
    }

    void applyCommands() {
        QCOMPARE(applyCommand(Backend::Awww, {}, {}), QStringLiteral("awww img %f"));
        QCOMPARE(applyCommand(Backend::Awww, QStringLiteral(" --transition-type grow "), {}),
                 QStringLiteral("awww img --transition-type grow %f"));
        QCOMPARE(applyCommand(Backend::Waypaper, QStringLiteral("--backend awww"), {}),
                 QStringLiteral("waypaper --backend awww --wallpaper %f"));
        QCOMPARE(applyCommand(Backend::Swaybg, QStringLiteral("-m fit"), {}),
                 QStringLiteral("pkill -x swaybg; setsid -f swaybg -m fill -m fit -i %f >/dev/null 2>&1"));
        QVERIFY(applyCommand(Backend::Hyprpaper, {}, {}).contains(QStringLiteral("wallpaper ,%f")));
        QCOMPARE(applyCommand(Backend::Command, {}, QStringLiteral("my-tool %f")), QStringLiteral("my-tool %f"));
        QVERIFY(applyCommand(Backend::None, {}, QStringLiteral("x")).isEmpty());
    }

    void awwwQuery() {
        const auto shown = parseQuery(
            u": DP-2: 1920x1080, scale: 1, currently displaying: image: /w/stairs.jpg\n"
            u": HDMI-A-1: 1920x1080, scale: 1, currently displaying: color: 000000\n"
            u": DP-3: 2560x1440, scale: 1.5, currently displaying: image: /w/my pics/a, b.png\n");
        QCOMPARE(shown, QStringList({QStringLiteral("/w/stairs.jpg"), QStringLiteral("/w/my pics/a, b.png")}));
    }

    void oldSwwwQuery() {
        QCOMPARE(parseQuery(u"eDP-1: 1920x1080, scale: 1, currently displaying: image: /w/a.jpg"),
                 QStringList({QStringLiteral("/w/a.jpg")}));
    }

    void hyprpaperQuery() {
        QCOMPARE(parseQuery(u"DP-2 = /w/a.jpg\nDP-3 = ~/w/b.jpg\n"),
                 QStringList({QStringLiteral("/w/a.jpg"), QStringLiteral("~/w/b.jpg")}));
        QVERIFY(parseQuery(u"no wallpapers active").isEmpty());
        QVERIFY(parseQuery(u"").isEmpty());
    }

    void waypaperIni() {
        const auto s = parseWaypaperIni(u"[Settings]\nlanguage = en\nfolder = ~/.config/wallpapers\n"
                                        u"monitors = All\nwallpaper = ~/.config/wallpapers/stairs.jpg\n"
                                        u"backend = awww\nfill = fill\n");
        QCOMPARE(s.wallpaper, QStringLiteral("~/.config/wallpapers/stairs.jpg"));
        QCOMPARE(s.backend, Backend::Awww);
        QVERIFY(!parseWaypaperIni(u"backend = mpvpaper\n").backend);
    }

    void swaybg() {
        QCOMPARE(swaybgImage({QStringLiteral("swaybg"), QStringLiteral("-m"), QStringLiteral("fill"),
                              QStringLiteral("-i"), QStringLiteral("/w/a.jpg")}),
                 QStringLiteral("/w/a.jpg"));
        QCOMPARE(swaybgImage({QStringLiteral("swaybg"), QStringLiteral("--image=/w/b.jpg")}), QStringLiteral("/w/b.jpg"));
        QVERIFY(swaybgImage({QStringLiteral("swaybg"), QStringLiteral("-c"), QStringLiteral("#000000")}).isEmpty());
    }

    void matchFollowsSymlinks() {
        QTemporaryDir dir;
        const auto real = dir.filePath(QStringLiteral("a.jpg"));
        QFile f(real);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.close();
        const auto link = dir.filePath(QStringLiteral("link.jpg"));
        QVERIFY(QFile::link(real, link));
        const QStringList files{dir.filePath(QStringLiteral("b.jpg")), real};

        QCOMPARE(match(files, {link}), real);
        QCOMPARE(match(files, {dir.path() + QStringLiteral("/./a.jpg")}), real);
        QCOMPARE(match(files, {QStringLiteral("/elsewhere.jpg"), real}), real);
        QVERIFY(match(files, {QStringLiteral("/elsewhere.jpg")}).isEmpty());
        QVERIFY(match(files, {}).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestWallpaperBackend)
#include "tst_wallpaperbackend.moc"
