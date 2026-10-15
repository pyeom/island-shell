#include "services/notifications/notificationrules.h"

#include <QTest>

#include <vector>

using namespace island;

class TestNotificationRules : public QObject {
    Q_OBJECT

private slots:
    void wildcard() {
        QVERIFY(wildcardMatch("discord", "Discord"));
        QVERIFY(wildcardMatch("*chrome*", "google-chrome-stable"));
        QVERIFY(wildcardMatch("org.kde.*", "org.kde.dolphin"));
        QVERIFY(!wildcardMatch("discord", "discord-canary"));
        QVERIFY(wildcardMatch("*", "anything"));
        QVERIFY(!wildcardMatch("a*b", "acd"));
    }

    void noRulesPopsUp() {
        const auto r = applyNotificationRules({}, "kitty", "", 1, false);
        QVERIFY(!r.drop);
        QVERIFY(r.popup);
        QCOMPARE(r.urgency, 1);
    }

    void muteDrops() {
        const std::vector<NotificationRule> rules = {{.app = "discord", .mute = true}};
        QVERIFY(applyNotificationRules(rules, "Discord", "", 1, false).drop);
    }

    void matchesDesktopEntry() {
        const std::vector<NotificationRule> rules = {{.app = "org.telegram.desktop", .noPopup = true}};
        const auto r = applyNotificationRules(rules, "Telegram", "org.telegram.desktop", 1, false);
        QVERIFY(!r.drop);
        QVERIFY(!r.popup);
    }

    void firstMatchWins() {
        const std::vector<NotificationRule> rules = {
            {.app = "spotify", .urgency = 0},
            {.app = "*", .mute = true},
        };
        const auto r = applyNotificationRules(rules, "Spotify", "", 1, false);
        QVERIFY(!r.drop);
        QCOMPARE(r.urgency, 0);
    }

    void dndLetsOnlyCriticalThrough() {
        QVERIFY(!applyNotificationRules({}, "app", "", 1, true).popup);
        QVERIFY(applyNotificationRules({}, "app", "", 2, true).popup);
        const std::vector<NotificationRule> rules = {{.app = "alarm", .urgency = 2}};
        QVERIFY(applyNotificationRules(rules, "alarm", "", 1, true).popup);
    }
};

QTEST_APPLESS_MAIN(TestNotificationRules)
#include "tst_notificationrules.moc"
