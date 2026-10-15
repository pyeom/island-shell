#include "configdata.h"

#include <algorithm>
#include <format>

namespace island {

namespace {

void readInt(const toml::table& t, std::string_view section, std::string_view key, int& out,
             int min, int max, std::vector<std::string>& errors) {
    const auto* node = t.get(key);
    if (!node)
        return;
    const auto v = node->value<int64_t>();
    if (!v) {
        errors.push_back(std::format("{}.{}: expected an integer", section, key));
        return;
    }
    if (*v < min || *v > max) {
        errors.push_back(std::format("{}.{}: {} is out of range [{}, {}]", section, key, *v, min, max));
        return;
    }
    out = static_cast<int>(*v);
}

void readString(const toml::table& t, std::string_view section, std::string_view key,
                std::string& out, std::initializer_list<std::string_view> allowed,
                std::vector<std::string>& errors) {
    const auto* node = t.get(key);
    if (!node)
        return;
    const auto v = node->value<std::string>();
    if (!v) {
        errors.push_back(std::format("{}.{}: expected a string", section, key));
        return;
    }
    if (allowed.size() > 0 && std::ranges::find(allowed, *v) == allowed.end()) {
        errors.push_back(std::format("{}.{}: unknown value \"{}\"", section, key, *v));
        return;
    }
    out = *v;
}

void readDouble(const toml::table& t, std::string_view section, std::string_view key, double& out,
                double min, double max, std::vector<std::string>& errors) {
    const auto* node = t.get(key);
    if (!node)
        return;
    const auto v = node->value<double>();
    if (!v || *v < min || *v > max) {
        errors.push_back(std::format("{}.{}: expected a number in [{}, {}]", section, key, min, max));
        return;
    }
    out = *v;
}

} // namespace

ConfigData parseConfig(std::string_view text, std::vector<std::string>& errors) {
    ConfigData cfg;
    try {
        cfg.raw = toml::parse(text);
    } catch (const toml::parse_error& e) {
        errors.push_back(std::format("line {}: {}", e.source().begin.line, e.description()));
        return cfg;
    }

    if (const auto* t = cfg.raw["island"].as_table()) {
        auto& i = cfg.island;
        readInt(*t, "island", "compact_width", i.compactWidth, 40, 2000, errors);
        readInt(*t, "island", "compact_height", i.compactHeight, 16, 200, errors);
        readInt(*t, "island", "peek_extra_width", i.peekExtraWidth, 0, 400, errors);
        readInt(*t, "island", "peek_extra_height", i.peekExtraHeight, 0, 100, errors);
        readInt(*t, "island", "expanded_width", i.expandedWidth, 40, 4000, errors);
        readInt(*t, "island", "expanded_height", i.expandedHeight, 16, 4000, errors);
        readInt(*t, "island", "expanded_radius", i.expandedRadius, 0, 200, errors);
        readInt(*t, "island", "top_margin", i.topMargin, 0, 200, errors);
        readInt(*t, "island", "exclusive_zone", i.exclusiveZone, 0, 400, errors);
        readInt(*t, "island", "surface_width", i.surfaceWidth, 100, 4000, errors);
        readInt(*t, "island", "surface_height", i.surfaceHeight, 50, 4000, errors);
    }

    // The surface never resizes, so it must fit the largest island state.
    auto& i = cfg.island;
    const int minSurfaceWidth = std::max({i.expandedWidth, i.compactWidth + i.peekExtraWidth});
    const int minSurfaceHeight =
        i.topMargin + std::max(i.expandedHeight, i.compactHeight + i.peekExtraHeight);
    if (i.surfaceWidth < minSurfaceWidth) {
        errors.push_back(std::format("island.surface_width raised to {} to fit the island",
                                     minSurfaceWidth));
        i.surfaceWidth = minSurfaceWidth;
    }
    if (i.surfaceHeight < minSurfaceHeight) {
        errors.push_back(std::format("island.surface_height raised to {} to fit the island",
                                     minSurfaceHeight));
        i.surfaceHeight = minSurfaceHeight;
    }

    if (const auto* t = cfg.raw["animations"].as_table()) {
        std::string mode = "full";
        readString(*t, "animations", "mode", mode, {"full", "reduced", "off"}, errors);
        cfg.animations = mode == "off"       ? AnimationMode::Off
                         : mode == "reduced" ? AnimationMode::Reduced
                                             : AnimationMode::Full;
    }

    if (const auto* t = cfg.raw["theme"].as_table()) {
        auto& th = cfg.theme;
        readString(*t, "theme", "source", th.source, {"static", "pywal"}, errors);
        readString(*t, "theme", "style", th.style, {"black", "blur"}, errors);
        readString(*t, "theme", "accent", th.accent, {}, errors);
        readString(*t, "theme", "pywal_accent", th.pywalAccent, {}, errors);
        readDouble(*t, "theme", "blur_opacity", th.blurOpacity, 0.0, 1.0, errors);
    }

    if (const auto* t = cfg.raw["clock"].as_table())
        readString(*t, "clock", "format", cfg.clock.format, {}, errors);

    if (const auto* t = cfg.raw["theme"].as_table())
        readString(*t, "theme", "icon_theme", cfg.iconTheme, {}, errors);

    if (const auto* t = cfg.raw["notifications"].as_table()) {
        auto& n = cfg.notifications;
        readInt(*t, "notifications", "timeout", n.timeout, 500, 600000, errors);
        readInt(*t, "notifications", "history_limit", n.historyLimit, 0, 10000, errors);
        if (const auto* node = t->get("rules")) {
            const auto* rules = node->as_array();
            if (!rules)
                errors.emplace_back("notifications.rules: expected an array of tables");
            for (size_t i = 0; rules && i < rules->size(); ++i) {
                const auto* rt = rules->get(i)->as_table();
                const auto where = std::format("notifications.rules[{}]", i);
                if (!rt) {
                    errors.push_back(where + ": expected a table");
                    continue;
                }
                NotificationRule rule;
                readString(*rt, where, "app", rule.app, {}, errors);
                if (rule.app.empty()) {
                    errors.push_back(where + ": missing \"app\"");
                    continue;
                }
                if (const auto v = (*rt)["mute"].value<bool>())
                    rule.mute = *v;
                if (const auto v = (*rt)["no_popup"].value<bool>())
                    rule.noPopup = *v;
                std::string urgency;
                readString(*rt, where, "urgency", urgency, {"low", "normal", "critical"}, errors);
                if (!urgency.empty())
                    rule.urgency = urgency == "low" ? 0 : urgency == "normal" ? 1 : 2;
                n.rules.push_back(std::move(rule));
            }
        }
    }

    if (const auto* t = cfg.raw["launcher"].as_table()) {
        readString(*t, "launcher", "terminal", cfg.launcher.terminal, {}, errors);
        readString(*t, "launcher", "launch_prefix", cfg.launcher.launchPrefix, {}, errors);
    }

    if (const auto* t = cfg.raw["calendar"].as_table()) {
        auto& c = cfg.calendar;
        if (const auto* node = t->get("paths")) {
            if (const auto* arr = node->as_array()) {
                c.paths.clear();
                for (const auto& item : *arr) {
                    if (const auto v = item.value<std::string>())
                        c.paths.push_back(*v);
                    else
                        errors.emplace_back("calendar.paths: expected strings");
                }
            } else {
                errors.emplace_back("calendar.paths: expected an array of strings");
            }
        }
        readInt(*t, "calendar", "days", c.days, 1, 730, errors);
        readInt(*t, "calendar", "reminder_minutes", c.reminderMinutes, 0, 1440, errors);
    }

    if (const auto* t = cfg.raw["power"].as_table()) {
        readString(*t, "power", "lock_command", cfg.power.lockCommand, {}, errors);
        readInt(*t, "power", "hold_ms", cfg.power.holdMs, 0, 5000, errors);
    }

    return cfg;
}

} // namespace island
