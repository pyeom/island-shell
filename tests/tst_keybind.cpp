#include "services/hyprland/keybind.h"

#include <QTest>

using namespace island::hypr;

class TestKeybind : public QObject {
    Q_OBJECT

private slots:
    void parsesCombos() {
        const auto c = parseKeyCombo(u"shift+super + r");
        QVERIFY(c);
        QCOMPARE(c->mods, QStringList({QStringLiteral("SUPER"), QStringLiteral("SHIFT")}));
        QCOMPARE(c->key, QStringLiteral("R"));
        QCOMPARE(comboText(*c), QStringLiteral("SUPER + SHIFT + R"));
        QCOMPARE(modMask(*c), 65);

        QCOMPARE(comboText(*parseKeyCombo(u"print")), QStringLiteral("Print"));
        QCOMPARE(comboText(*parseKeyCombo(u"ALT + tab")), QStringLiteral("ALT + Tab"));
        QCOMPARE(comboText(*parseKeyCombo(u"control+mod1+win+SPACE")), QStringLiteral("SUPER + CTRL + ALT + space"));
        QCOMPARE(comboText(*parseKeyCombo(u"SUPER + XF86AudioPlay")), QStringLiteral("SUPER + XF86AudioPlay"));
        QCOMPARE(comboText(*parseKeyCombo(u"SUPER + code:28")), QStringLiteral("SUPER + code:28"));
        QCOMPARE(comboText(*parseKeyCombo(u"SUPER + 1")), QStringLiteral("SUPER + 1"));
    }

    void rejectsBadCombos() {
        QString error;
        QVERIFY(!parseKeyCombo(u"HYPER + R", &error));
        QCOMPARE(error, QStringLiteral("unknown modifier \"HYPER\""));
        QVERIFY(!parseKeyCombo(u"SUPER + NOTAKEY", &error));
        QCOMPARE(error, QStringLiteral("unknown key \"NOTAKEY\""));
        QVERIFY(!parseKeyCombo(u"SUPER +", &error));
        QVERIFY(!parseKeyCombo(u"", &error));
        QCOMPARE(error, QStringLiteral("no key"));
        QVERIFY(!parseKeyCombo(u"SUPER + code:x"));
    }

    void bindCommands() {
        const auto c = *parseKeyCombo(u"SUPER + SHIFT + R");
        QCOMPARE(luaUnbind(c), QStringLiteral("hl.unbind([[SUPER + SHIFT + R]])"));
        QCOMPARE(luaBind(c, QStringLiteral("islandctl launcher toggle"), QStringLiteral("island-shell: launcher")),
                 QStringLiteral("hl.unbind([[SUPER + SHIFT + R]])\n"
                                "hl.bind([[SUPER + SHIFT + R]], hl.dsp.exec_cmd([[islandctl launcher toggle]]), "
                                "{ description = [[island-shell: launcher]] })"));
        QCOMPARE(confUnbind(c), QStringLiteral("SUPER SHIFT,R"));
        QCOMPARE(confBind(c, QStringLiteral("islandctl a, b"), QStringLiteral("x, y")),
                 QStringLiteral("SUPER SHIFT,R,x  y,exec,islandctl a, b"));
        QCOMPARE(confUnbind(*parseKeyCombo(u"Print")), QStringLiteral(",Print"));
    }

    void boundKeys() {
        const auto binds = parseBinds(R"([
            {"modmask": 64, "key": "R", "submap": "", "description": ""},
            {"modmask": 8, "key": "TAB", "submap": "", "description": "island-shell: power"},
            {"modmask": 0, "key": "Escape", "submap": "island-shell-record", "description": ""}])");
        QCOMPARE(binds.size(), 3);
        QVERIFY(sameKeys(binds[0], *parseKeyCombo(u"SUPER + r")));
        QVERIFY(!sameKeys(binds[0], *parseKeyCombo(u"SUPER + SHIFT + R")));
        QVERIFY(sameKeys(binds[1], *parseKeyCombo(u"ALT + Tab")));
        QCOMPARE(binds[1].description, QStringLiteral("island-shell: power"));
        QVERIFY(!sameKeys(binds[2], *parseKeyCombo(u"Escape"))); // other submap
    }

    void mainKeyboard() {
        const auto names = parseMainKeyboard(R"({"keyboards": [
            {"name": "a", "layout": "us", "main": false},
            {"name": "b", "layout": "latam", "variant": "", "options": "caps:escape", "main": true}]})");
        QCOMPARE(names.layout, QStringLiteral("latam"));
        QCOMPARE(names.options, QStringLiteral("caps:escape"));
    }

    void keycodes() {
        // xkb keycodes are evdev + 8.
        const XkbNames us{{}, {}, QStringLiteral("us"), {}, {}};
        QCOMPARE(keycodeName(us, 19 + 8), QStringLiteral("R"));
        QCOMPARE(keycodeName(us, 2 + 8), QStringLiteral("1")); // not "exclam" with Shift down
        QCOMPARE(keycodeName(us, 57 + 8), QStringLiteral("space"));
        QCOMPARE(keycodeName(us, 99 + 8), QStringLiteral("Print"));
        QVERIFY(keycodeName(us, 42 + 8).isEmpty());  // Shift_L
        QVERIFY(keycodeName(us, 125 + 8).isEmpty()); // Super_L
        const XkbNames latam{{}, {}, QStringLiteral("latam"), {}, {}};
        QCOMPARE(keycodeName(latam, 39 + 8), QStringLiteral("ntilde"));
    }

    void actions() {
        QVERIFY(keyAction("launcher"));
        QCOMPARE(keyAction("launcher")->args, "launcher toggle");
        QVERIFY(!keyAction("nope"));
    }
};

QTEST_GUILESS_MAIN(TestKeybind)
#include "tst_keybind.moc"
