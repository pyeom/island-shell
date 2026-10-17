#include "arbiter.h"

#include "activity.h"
#include "activityregistry.h"
#include "config.h"
#include "selection.h"

#include <vector>

namespace island {

Arbiter::Arbiter(QObject* parent) : QObject(parent) {
    connect(ActivityRegistry::instance(), &ActivityRegistry::changed, this, &Arbiter::update);
    connect(Config::instance(), &Config::changed, this, &Arbiter::update);
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

int Arbiter::priority(const Activity* activity) {
    // [priorities] cannot lower critical notifications or modals (>= 100).
    const int own = activity->priority();
    if (own >= 100)
        return own;
    const auto& overrides = Config::instance()->data().priorities;
    const auto name = activity->name().toStdString();
    auto it = overrides.find(name);
    if (it == overrides.end())
        it = overrides.find(name.substr(0, name.find(':'))); // "live:<id>" → "live"
    return it != overrides.end() ? it->second : own;
}

void Arbiter::update() {
    const auto activities = ActivityRegistry::instance()->activities();
    std::vector<Candidate> candidates;
    candidates.reserve(activities.size());
    for (const auto* a : activities) {
        candidates.push_back({
            .priority = priority(a),
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
