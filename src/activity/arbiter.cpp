#include "arbiter.h"

#include "activity.h"
#include "activityregistry.h"
#include "selection.h"

#include <vector>

namespace island {

Arbiter::Arbiter(QObject* parent) : QObject(parent) {
    connect(ActivityRegistry::instance(), &ActivityRegistry::changed, this, &Arbiter::update);
    update();
}

void Arbiter::setFocused(bool focused) {
    if (m_focused == focused)
        return;
    m_focused = focused;
    emit focusedChanged();
    update();
}

bool Arbiter::eligible(Activity* activity) const {
    return activity && activity->active() &&
           (activity->scope() == Activity::AllScreens || m_focused);
}

void Arbiter::update() {
    const auto activities = ActivityRegistry::instance()->activities();
    std::vector<Candidate> candidates;
    candidates.reserve(activities.size());
    for (const auto* a : activities) {
        candidates.push_back({
            .priority = a->priority(),
            .active = a->active(),
            .allScreens = a->scope() == Activity::AllScreens,
            .hasMinimal = a->minimal() != nullptr,
        });
    }
    const auto sel = selectActivities(candidates, m_focused);
    Activity* primary = sel.primary >= 0 ? activities[sel.primary] : nullptr;
    Activity* secondary = sel.secondary >= 0 ? activities[sel.secondary] : nullptr;
    if (primary == m_primary && secondary == m_secondary)
        return;
    m_primary = primary;
    m_secondary = secondary;
    emit selectionChanged();
}

} // namespace island
