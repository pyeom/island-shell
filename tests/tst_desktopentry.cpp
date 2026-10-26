#include "services/apps/desktopentry.h"

#include <QTest>

using namespace island;

namespace {
const QString kFirefox = QStringLiteral(
    "[Desktop Entry]\n"
    "Type=Application\n"
    "Name=Firefox\n"
    "Name[es]=Navegador Firefox\n"
    "GenericName=Web Browser\n"
    "Keywords=Internet;WWW;Browser\\;Web;\n"
    "Exec=/usr/lib/firefox/firefox %u\n"
    "Icon=firefox\n"
    "MimeType=text/html;x-scheme-handler/http;\n"
    "Categories=Network;WebBrowser;\n"
    "Actions=new-window;private;\n"
    "\n"
    "[Desktop Action new-window]\n"
    "Name=New Window\n"
    "Name[es]=Nueva ventana\n"
    "Exec=/usr/lib/firefox/firefox --new-window %u\n"
    "\n"
    "[Desktop Action private]\n"
    "Name=New Private Window\n"
    "Exec=/usr/lib/firefox/firefox --private-window %u\n");
} // namespace

class TestDesktopEntry : public QObject {
    Q_OBJECT

private slots:
    void commandPrograms() {
        QCOMPARE(commandProgram(QStringLiteral("yay -Syu")), QStringLiteral("yay"));
        QCOMPARE(commandProgram(QStringLiteral("GTK_THEME=Adwaita env /usr/bin/firefox -P x")), QStringLiteral("firefox"));
        QCOMPARE(commandProgram(QStringLiteral("  ")), QString());
    }

    void graphicalCommands() {
        auto entry = [](const char* exec, bool terminal) {
            DesktopEntry e;
            e.exec = QString::fromUtf8(exec);
            e.terminal = terminal;
            return e;
        };
        const QList<DesktopEntry> entries{entry("/usr/lib/firefox/firefox %u", false),
                                          entry("htop", true),
                                          entry("flatpak run org.gimp.GIMP %U", false),
                                          entry("env GDK_BACKEND=x11 steam %U", false)};
        QVERIFY(isGraphicalCommand(QStringLiteral("firefox --new-window"), entries));
        QVERIFY(isGraphicalCommand(QStringLiteral("steam"), entries));
        QVERIFY(!isGraphicalCommand(QStringLiteral("yay"), entries));
        QVERIFY(!isGraphicalCommand(QStringLiteral("htop"), entries));          // Terminal=true
        QVERIFY(!isGraphicalCommand(QStringLiteral("flatpak update"), entries)); // a wrapper
        QVERIFY(!isGraphicalCommand(QString(), entries));
    }

    void parsesEntry() {
        const auto e = parseDesktopEntry(kFirefox, QStringLiteral("en_US"));
        QVERIFY(e);
        QCOMPARE(e->name, QStringLiteral("Firefox"));
        QCOMPARE(e->genericName, QStringLiteral("Web Browser"));
        QCOMPARE(e->keywords, QStringList({"Internet", "WWW", "Browser;Web"}));
        QCOMPARE(e->icon, QStringLiteral("firefox"));
        QCOMPARE(e->mimeTypes, QStringList({"text/html", "x-scheme-handler/http"}));
        QCOMPARE(e->categories, QStringList({"Network", "WebBrowser"}));
        QCOMPARE(e->actions.size(), 2);
        QCOMPARE(e->actions[1].id, QStringLiteral("private"));
    }

    void localizedKeys() {
        const auto e = parseDesktopEntry(kFirefox, QStringLiteral("es_CL"));
        QVERIFY(e);
        QCOMPARE(e->name, QStringLiteral("Navegador Firefox"));
        QCOMPARE(e->actions[0].name, QStringLiteral("Nueva ventana"));
        QCOMPARE(e->actions[1].name, QStringLiteral("New Private Window"));
    }

    void rejectsNonApplications() {
        QVERIFY(!parseDesktopEntry(QStringLiteral("[Desktop Entry]\nType=Link\nName=x\nURL=y\n"), {}));
        QVERIFY(!parseDesktopEntry(QStringLiteral("[Desktop Entry]\nType=Application\nName=x\n"), {}));
        QVERIFY(!parseDesktopEntry(QStringLiteral("garbage"), {}));
    }

    void visibility() {
        auto e = *parseDesktopEntry(kFirefox, {});
        const QStringList hypr = {QStringLiteral("Hyprland")};
        QVERIFY(e.visibleIn(hypr));
        e.onlyShowIn = {QStringLiteral("KDE")};
        QVERIFY(!e.visibleIn(hypr));
        e.onlyShowIn.clear();
        e.notShowIn = {QStringLiteral("hyprland")};
        QVERIFY(!e.visibleIn(hypr));
        e.notShowIn.clear();
        e.noDisplay = true;
        QVERIFY(!e.visibleIn(hypr));
    }

    void fieldCodes() {
        QCOMPARE(stripFieldCodes(QStringLiteral("firefox %u")), QStringLiteral("firefox"));
        QCOMPARE(stripFieldCodes(QStringLiteral("app --x %F --name %c")),
                 QStringLiteral("app --x --name"));
        QCOMPARE(stripFieldCodes(QStringLiteral("echo 100%%")), QStringLiteral("echo 100%"));
    }

    void quoting() {
        QCOMPARE(shellQuote(QStringLiteral("a b")), QStringLiteral("'a b'"));
        QCOMPARE(shellQuote(QStringLiteral("it's")), QStringLiteral("'it'\\''s'"));
        QCOMPARE(shellQuote(QStringLiteral("$(rm -rf ~)")), QStringLiteral("'$(rm -rf ~)'"));
    }

    void wmClass() {
        const auto e = parseDesktopEntry(QStringLiteral("[Desktop Entry]\nType=Application\nName=Code\n"
                                                        "Exec=code\nStartupWMClass=Code\n"),
                                         {});
        QVERIFY(e);
        QCOMPARE(e->startupWmClass, QStringLiteral("Code"));
    }

    void fuzzy() {
        QVERIFY(fuzzyScore(u"ff", u"Firefox") > 0);
        QCOMPARE(fuzzyScore(u"xyz", u"Firefox"), -1);
        // Prefix beats a match in the middle.
        QVERIFY(fuzzyScore(u"fi", u"Firefox") > fuzzyScore(u"fi", u"Profile Manager"));
        // Literal words beat scattered letters.
        QVERIFY(fuzzyScore(u"fire", u"fire") > fuzzyScore(u"fire", u"flag: Ireland"));
        QVERIFY(fuzzyScore(u"term", u"Konsole terminal") > fuzzyScore(u"term", u"Tetris Emulator"));
        // Word starts count: "vsc" in "Visual Studio Code".
        QVERIFY(fuzzyScore(u"vsc", u"Visual Studio Code") > fuzzyScore(u"vsc", u"Avisucer"));
    }

    void ranking() {
        const auto e = *parseDesktopEntry(kFirefox, {});
        QVERIFY(rankEntry(e, u"web", 0) > 0);   // generic name
        QVERIFY(rankEntry(e, u"www", 0) > 0);   // keyword
        QCOMPARE(rankEntry(e, u"zzz", 0), -1);
        QVERIFY(rankEntry(e, u"fire", 50) > rankEntry(e, u"fire", 0));
        // Empty query: the plain count, so 7 and 8 uses do not tie.
        QVERIFY(rankEntry(e, u"", 8) > rankEntry(e, u"", 7));
        QCOMPARE(rankEntry(e, u"", 0), 0);
    }
};

QTEST_APPLESS_MAIN(TestDesktopEntry)
#include "tst_desktopentry.moc"
