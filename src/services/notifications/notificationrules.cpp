#include "notificationrules.h"

#include <cctype>

namespace island {

namespace {
bool equalNoCase(char a, char b) {
    return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
}
} // namespace

bool wildcardMatch(std::string_view pattern, std::string_view text) {
    // Iterative glob with single-star backtracking.
    size_t p = 0;
    size_t t = 0;
    size_t star = std::string_view::npos;
    size_t mark = 0;
    while (t < text.size()) {
        if (p < pattern.size() && pattern[p] == '*') {
            star = p++;
            mark = t;
        } else if (p < pattern.size() && equalNoCase(pattern[p], text[t])) {
            ++p;
            ++t;
        } else if (star != std::string_view::npos) {
            p = star + 1;
            t = ++mark;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*')
        ++p;
    return p == pattern.size();
}

RuleResult applyNotificationRules(std::span<const NotificationRule> rules, std::string_view appName,
                                  std::string_view desktopEntry, int urgency, bool dnd) {
    RuleResult result;
    result.urgency = urgency;
    for (const auto& rule : rules) {
        const bool matches = wildcardMatch(rule.app, appName) ||
                             (!desktopEntry.empty() && wildcardMatch(rule.app, desktopEntry));
        if (!matches)
            continue;
        if (rule.mute) {
            result.drop = true;
            return result;
        }
        result.popup = !rule.noPopup;
        if (rule.urgency >= 0)
            result.urgency = rule.urgency;
        break;
    }
    if (dnd && result.urgency < 2)
        result.popup = false;
    return result;
}

} // namespace island
