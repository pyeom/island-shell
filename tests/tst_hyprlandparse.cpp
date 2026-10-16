#include "services/hyprland/hyprlandparse.h"

#include <QTest>

using namespace island::hypr;

class TestHyprlandParse : public QObject {
    Q_OBJECT

private slots:
    void eventLine() {
        const auto ev = parseEventLine("focusedmon>>DP-3,2");
        QVERIFY(ev);
        QCOMPARE(ev->name, QStringLiteral("focusedmon"));
        QCOMPARE(ev->data, QStringLiteral("DP-3,2"));
    }

    void eventDataMayContainSeparator() {
        const auto ev = parseEventLine("activewindow>>kitty,a >> b");
        QVERIFY(ev);
        QCOMPARE(ev->data, QStringLiteral("kitty,a >> b"));
    }

    void malformedEvent() {
        QVERIFY(!parseEventLine("garbage"));
        QVERIFY(!parseEventLine(">>no-name"));
    }

    void workspaceEventsTriggerRefresh() {
        QVERIFY(affectsWorkspaces(QStringLiteral("workspacev2")));
        QVERIFY(affectsWorkspaces(QStringLiteral("focusedmon")));
        QVERIFY(affectsWorkspaces(QStringLiteral("fullscreen")));
        QVERIFY(!affectsWorkspaces(QStringLiteral("activewindow")));
    }

    void monitors() {
        QString focused;
        const auto list = parseMonitors(R"([
            {"id": 1, "name": "DP-3", "focused": false, "activeWorkspace": {"id": 2, "name": "2"}},
            {"id": 0, "name": "DP-2", "focused": true, "activeWorkspace": {"id": 1, "name": "1"}}
        ])",
                                        &focused);
        QCOMPARE(focused, QStringLiteral("DP-2"));
        QCOMPARE(list.size(), 2);
        const auto first = list[0].toMap();
        QCOMPARE(first[QStringLiteral("name")].toString(), QStringLiteral("DP-2"));
        QCOMPARE(first[QStringLiteral("activeWorkspace")].toInt(), 1);
    }

    void workspacesSkipSpecialAndSort() {
        const auto list = parseWorkspaces(R"([
            {"id": 3, "name": "3", "monitor": "HDMI-A-1", "windows": 2, "hasfullscreen": true},
            {"id": -98, "name": "special:scratch", "monitor": "DP-2", "windows": 1},
            {"id": 1, "name": "1", "monitor": "DP-2", "windows": 1}
        ])");
        QCOMPARE(list.size(), 2);
        QCOMPARE(list[0].toMap()[QStringLiteral("id")].toInt(), 1);
        QCOMPARE(list[1].toMap()[QStringLiteral("monitor")].toString(), QStringLiteral("HDMI-A-1"));
        QVERIFY(list[1].toMap()[QStringLiteral("hasFullscreen")].toBool());
        QVERIFY(!list[0].toMap()[QStringLiteral("hasFullscreen")].toBool());
    }

    void dispatchSyntaxes() {
        const auto e = execDispatch(QStringLiteral("$(pidof -q rofi) && pkill rofi || echo \"a]]b\""));
        QCOMPARE(e.legacy, QStringLiteral("exec $(pidof -q rofi) && pkill rofi || echo \"a]]b\""));
        QCOMPARE(e.lua, QStringLiteral("hl.dsp.exec_cmd([=[$(pidof -q rofi) && pkill rofi || echo \"a]]b\"]=])"));
        QCOMPARE(workspaceDispatch(3).lua, QStringLiteral("hl.dsp.focus({ workspace = 3 })"));
        QCOMPARE(workspaceDispatch(3).legacy, QStringLiteral("workspace 3"));
        QCOMPARE(focusWindowDispatch(QStringLiteral("0x55")).lua,
                 QStringLiteral("hl.dsp.focus({ window = [[address:0x55]] })"));
        QCOMPARE(exitDispatch().lua, QStringLiteral("hl.dsp.exit()"));
        QCOMPARE(luaString(u"x]=]y]]"), QStringLiteral("[==[x]=]y]]]==]"));
        QCOMPARE(luaString(u"\nz"), QStringLiteral("[[\n\nz]]"));
    }

    void openWindowEvent() {
        QCOMPARE(openWindowClass(u"55a1b2c3,1,firefox,YouTube, the video"), QStringLiteral("firefox"));
        QCOMPARE(openWindowClass(u"55a1b2c3,1,,"), QString());
        QCOMPARE(openWindowClass(u"garbage"), QString());
    }

    void luaSyntaxError() {
        QVERIFY(isLuaSyntaxError("error: [string \"return hl.dispatch(exec true)\"]:1: ')' expected near 'true'\n\n"
                                 " \xe2\x86\x92 Note: dispatch in lua is a shorthand for hl.dispatch(...)"));
        QVERIFY(!isLuaSyntaxError("ok"));
        QVERIFY(!isLuaSyntaxError("Invalid dispatcher"));
    }

    void maximizedIsNotFullscreen() {
        const QByteArray workspaces = R"([
            {"id": 1, "name": "1", "monitor": "DP-2", "windows": 1, "hasfullscreen": true},
            {"id": 2, "name": "2", "monitor": "DP-3", "windows": 1, "hasfullscreen": true},
            {"id": 3, "name": "3", "monitor": "DP-3", "windows": 1, "hasfullscreen": true}
        ])";
        const QByteArray clients = R"([
            {"address": "0x1", "workspace": {"id": 1, "name": "1"}, "fullscreen": 1},
            {"address": "0x2", "workspace": {"id": 2, "name": "2"}, "fullscreen": 2},
            {"address": "0x3", "workspace": {"id": 3, "name": "3"}, "fullscreen": 3}
        ])";
        const auto list = parseWorkspaces(workspaces, clients);
        QVERIFY(!list[0].toMap()[QStringLiteral("hasFullscreen")].toBool());
        QVERIFY(list[1].toMap()[QStringLiteral("hasFullscreen")].toBool());
        QVERIFY(list[2].toMap()[QStringLiteral("hasFullscreen")].toBool());
    }

    void clients() {
        const auto list = parseClients(R"([
            {"address": "0x1", "class": "firefox", "title": "Twitch", "mapped": true, "hidden": false,
             "workspace": {"id": 2, "name": "2"}, "monitor": 1},
            {"address": "0x2", "class": "kitty", "title": "x", "mapped": false, "hidden": false,
             "workspace": {"id": 1, "name": "1"}, "monitor": 0},
            {"address": "0x3", "class": "", "title": "", "mapped": true, "hidden": false,
             "workspace": {"id": 1, "name": "1"}, "monitor": 0}
        ])");
        QCOMPARE(list.size(), 1);
        const auto c = list[0].toMap();
        QCOMPARE(c[QStringLiteral("class")].toString(), QStringLiteral("firefox"));
        QCOMPARE(c[QStringLiteral("workspace")].toString(), QStringLiteral("2"));
    }

    void invalidJson() {
        QVERIFY(parseWorkspaces("not json").isEmpty());
        QVERIFY(parseMonitors("", nullptr).isEmpty());
    }
};

QTEST_APPLESS_MAIN(TestHyprlandParse)
#include "tst_hyprlandparse.moc"
