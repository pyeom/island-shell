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
            {"id": 3, "name": "3", "monitor": "HDMI-A-1", "windows": 2},
            {"id": -98, "name": "special:scratch", "monitor": "DP-2", "windows": 1},
            {"id": 1, "name": "1", "monitor": "DP-2", "windows": 1}
        ])");
        QCOMPARE(list.size(), 2);
        QCOMPARE(list[0].toMap()[QStringLiteral("id")].toInt(), 1);
        QCOMPARE(list[1].toMap()[QStringLiteral("monitor")].toString(), QStringLiteral("HDMI-A-1"));
    }

    void invalidJson() {
        QVERIFY(parseWorkspaces("not json").isEmpty());
        QVERIFY(parseMonitors("", nullptr).isEmpty());
    }
};

QTEST_APPLESS_MAIN(TestHyprlandParse)
#include "tst_hyprlandparse.moc"
