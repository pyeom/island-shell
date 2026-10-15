#pragma once

#include "configdata.h"

#include <span>
#include <string_view>

namespace island {

struct RuleResult {
    bool drop = false;  // muted: not shown, not stored
    bool popup = true;  // false: history only
    int urgency = 1;
};

// Case-insensitive match with '*' wildcards ("discord", "*chrome*").
bool wildcardMatch(std::string_view pattern, std::string_view text);

// Applies the first matching rule (by app name or desktop entry), then DND:
// with do-not-disturb only critical notifications pop up.
RuleResult applyNotificationRules(std::span<const NotificationRule> rules, std::string_view appName,
                                  std::string_view desktopEntry, int urgency, bool dnd);

} // namespace island
