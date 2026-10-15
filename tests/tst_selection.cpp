#include "activity/selection.h"

#include <QTest>

#include <vector>

using island::Candidate;
using island::selectActivities;

class TestSelection : public QObject {
    Q_OBJECT

private slots:
    void emptyHasNoSelection() {
        const auto sel = selectActivities({}, true);
        QCOMPARE(sel.primary, -1);
        QCOMPARE(sel.secondary, -1);
    }

    void highestPriorityWins() {
        const std::vector<Candidate> c = {
            {.priority = 0, .active = true, .allScreens = true},
            {.priority = 50, .active = true, .hasMinimal = true},
            {.priority = 90, .active = true, .hasMinimal = true},
        };
        const auto sel = selectActivities(c, true);
        QCOMPARE(sel.primary, 2);
        QCOMPARE(sel.secondary, 1);
    }

    void inactiveIsIgnored() {
        const std::vector<Candidate> c = {
            {.priority = 0, .active = true, .allScreens = true},
            {.priority = 90, .active = false, .hasMinimal = true},
        };
        QCOMPARE(selectActivities(c, true).primary, 0);
    }

    // Notifications and other per-focus activities stay off other monitors.
    void focusedScopeOnlyOnFocusedMonitor() {
        const std::vector<Candidate> c = {
            {.priority = 0, .active = true, .allScreens = true},
            {.priority = 90, .active = true, .allScreens = false, .hasMinimal = true},
        };
        QCOMPARE(selectActivities(c, false).primary, 0);
        QCOMPARE(selectActivities(c, true).primary, 1);
    }

    void secondaryNeedsMinimal() {
        const std::vector<Candidate> c = {
            {.priority = 10, .active = true, .hasMinimal = false},
            {.priority = 90, .active = true, .hasMinimal = true},
        };
        const auto sel = selectActivities(c, true);
        QCOMPARE(sel.primary, 1);
        QCOMPARE(sel.secondary, -1);
    }

    void tiesGoToEarlierRegistration() {
        const std::vector<Candidate> c = {
            {.priority = 50, .active = true, .hasMinimal = true},
            {.priority = 50, .active = true, .hasMinimal = true},
            {.priority = 50, .active = true, .hasMinimal = true},
        };
        const auto sel = selectActivities(c, true);
        QCOMPARE(sel.primary, 0);
        QCOMPARE(sel.secondary, 1);
    }
};

QTEST_APPLESS_MAIN(TestSelection)
#include "tst_selection.moc"
