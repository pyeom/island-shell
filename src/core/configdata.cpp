#include "configdata.h"

#include "keybind.h"

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

void readBool(const toml::table& t, std::string_view section, std::string_view key, bool& out,
              std::vector<std::string>& errors) {
    const auto* node = t.get(key);
    if (!node)
        return;
    if (const auto v = node->value<bool>())
        out = *v;
    else
        errors.push_back(std::format("{}.{}: expected true or false", section, key));
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

std::string searchEngineUrl(std::string_view engine) {
    static constexpr std::pair<std::string_view, std::string_view> kEngines[] = {
        {"duckduckgo", "https://duckduckgo.com/?q=%s"},
        {"google", "https://www.google.com/search?q=%s"},
        {"bing", "https://www.bing.com/search?q=%s"},
        {"brave", "https://search.brave.com/search?q=%s"},
        {"startpage", "https://www.startpage.com/do/search?q=%s"},
        {"ecosia", "https://www.ecosia.org/search?q=%s"},
        {"kagi", "https://kagi.com/search?q=%s"},
    };
    for (const auto& [name, url] : kEngines) {
        if (name == engine)
            return std::string(url);
    }
    return {};
}

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
        readString(*t, "island", "fullscreen", i.fullscreen, {"minimal", "hide", "show"}, errors);
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
        auto& l = cfg.launcher;
        std::string custom;
        readString(*t, "launcher", "search_url", custom, {}, errors);
        // Older configs only had search_url: a URL of its own means custom.
        if (!t->contains("search_engine") && !custom.empty() && custom != l.searchUrl)
            l.searchEngine = "custom";
        readString(*t, "launcher", "search_engine", l.searchEngine,
                   {"duckduckgo", "google", "bing", "brave", "startpage", "ecosia", "kagi", "custom"}, errors);
        if (l.searchEngine != "custom") {
            l.searchUrl = searchEngineUrl(l.searchEngine);
        } else if (custom.find("%s") != std::string::npos) {
            l.searchUrl = custom;
        } else {
            errors.emplace_back("launcher.search_url: must contain %s");
        }
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
        readString(*t, "calendar", "default", c.defaultCalendar, {}, errors);
    }

    if (const auto* t = cfg.raw["system"].as_table()) {
        readInt(*t, "system", "cpu_hot", cfg.system.cpuHot, 0, 150, errors);
        readInt(*t, "system", "gpu_hot", cfg.system.gpuHot, 0, 150, errors);
        readInt(*t, "system", "nvme_hot", cfg.system.nvmeHot, 0, 150, errors);
    }

    if (const auto* t = cfg.raw["weather"].as_table()) {
        readString(*t, "weather", "city", cfg.weather.city, {}, errors);
        readString(*t, "weather", "units", cfg.weather.units, {"metric", "imperial"}, errors);
        readInt(*t, "weather", "rain_alert", cfg.weather.rainAlert, 0, 100, errors);
    }

    if (const auto* t = cfg.raw["timer"].as_table()) {
        readString(*t, "timer", "sound", cfg.timer.sound, {}, errors);
        readDouble(*t, "timer", "volume", cfg.timer.volume, 0.0, 1.0, errors);
    }

    if (const auto* t = cfg.raw["updates"].as_table()) {
        readString(*t, "updates", "command", cfg.updates.command, {}, errors);
        readInt(*t, "updates", "interval", cfg.updates.interval, 0, 10080, errors);
    }

    if (const auto* t = cfg.raw["night_light"].as_table())
        readString(*t, "night_light", "command", cfg.nightLight.command, {}, errors);

    if (const auto* t = cfg.raw["screenshots"].as_table()) {
        readString(*t, "screenshots", "directory", cfg.screenshots.directory, {}, errors);
        readString(*t, "screenshots", "editor", cfg.screenshots.editor, {}, errors);
    }

    if (const auto* t = cfg.raw["wallpaper"].as_table()) {
        readString(*t, "wallpaper", "directory", cfg.wallpaper.directory, {}, errors);
        readString(*t, "wallpaper", "backend", cfg.wallpaper.backend,
                   {"auto", "awww", "swww", "hyprpaper", "swaybg", "waypaper", "command"}, errors);
        readString(*t, "wallpaper", "backend_args", cfg.wallpaper.backendArgs, {}, errors);
        readBool(*t, "wallpaper", "pywal", cfg.wallpaper.pywal, errors);
        readString(*t, "wallpaper", "apply", cfg.wallpaper.apply, {}, errors);
    }

    if (const auto* t = cfg.raw["keybinds"].as_table()) {
        for (const auto& [name, node] : *t) {
            const std::string action(name.str());
            const auto where = "keybinds." + action;
            if (!hypr::keyAction(action)) {
                errors.push_back(where + ": unknown action");
                continue;
            }
            const auto v = node.value<std::string>();
            if (!v) {
                errors.push_back(where + ": expected a string such as \"SUPER + R\"");
                continue;
            }
            if (v->find_first_not_of(" \t") == std::string::npos)
                continue; // "" leaves it unbound
            QString why;
            const auto combo = hypr::parseKeyCombo(QString::fromStdString(*v), &why);
            if (!combo) {
                errors.push_back(std::format("{}: {}", where, why.toStdString()));
                continue;
            }
            const auto keys = hypr::comboText(*combo).toStdString();
            const auto taken = std::ranges::find(cfg.keybinds, keys, &Keybind::keys);
            if (taken != cfg.keybinds.end()) {
                errors.push_back(std::format("{}: {} is already used by keybinds.{}", where, keys, taken->action));
                continue;
            }
            cfg.keybinds.push_back({action, keys});
        }
    }

    if (const auto* t = cfg.raw["live"].as_table()) {
        readBool(*t, "live", "app_progress", cfg.live.appProgress, errors);
        readBool(*t, "live", "downloads", cfg.live.downloads, errors);
        readString(*t, "live", "downloads_directory", cfg.live.downloadsDirectory, {}, errors);
    }

    if (const auto* t = cfg.raw["privacy"].as_table()) {
        readBool(*t, "privacy", "microphone", cfg.privacy.microphone, errors);
        readBool(*t, "privacy", "camera", cfg.privacy.camera, errors);
        readBool(*t, "privacy", "screen", cfg.privacy.screen, errors);
    }

    if (const auto* t = cfg.raw["priorities"].as_table()) {
        for (const auto& [name, node] : *t) {
            const std::string key(name.str());
            int priority = -1;
            readInt(*t, "priorities", key, priority, 0, 99, errors);
            if (priority >= 0)
                cfg.priorities[key] = priority;
        }
    }

    if (const auto* t = cfg.raw["power"].as_table()) {
        readString(*t, "power", "lock_command", cfg.power.lockCommand, {}, errors);
        readInt(*t, "power", "hold_ms", cfg.power.holdMs, 0, 5000, errors);
    }

    return cfg;
}

} // namespace island
