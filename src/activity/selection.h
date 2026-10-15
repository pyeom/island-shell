#pragma once

#include <span>

namespace island {

// What the arbiter needs to know about one registered activity.
struct Candidate {
    int priority = 0;
    bool active = false;
    bool allScreens = false; // false: only shown on the focused monitor
    bool hasMinimal = false; // can be shown as the secondary bubble
};

struct Selection {
    int primary = -1;   // index into the candidate list, -1 for none
    int secondary = -1; // -1 for none
};

// Picks the activity that owns the island and the one shown as the bubble.
// Higher priority wins; ties go to the earlier registration (lower index).
// The secondary is the best remaining candidate that has a minimal
// presentation.
inline Selection selectActivities(std::span<const Candidate> candidates, bool focused) {
    Selection sel;
    auto eligible = [&](const Candidate& c) { return c.active && (c.allScreens || focused); };
    auto better = [&](int a, int b) {
        return b < 0 || candidates[a].priority > candidates[b].priority;
    };

    for (int i = 0; i < static_cast<int>(candidates.size()); ++i) {
        if (eligible(candidates[i]) && better(i, sel.primary))
            sel.primary = i;
    }
    for (int i = 0; i < static_cast<int>(candidates.size()); ++i) {
        if (i != sel.primary && eligible(candidates[i]) && candidates[i].hasMinimal &&
            better(i, sel.secondary))
            sel.secondary = i;
    }
    return sel;
}

} // namespace island
