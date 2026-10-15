#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <toml++/toml.hpp>

namespace island {

enum class AnimationMode { Full, Reduced, Off };

struct IslandConfig {
    int compactWidth = 190;
    int compactHeight = 36;
    int peekExtraWidth = 40;
    int peekExtraHeight = 6;
    int expandedWidth = 420;
    int expandedHeight = 200;
    int expandedRadius = 34;
    int topMargin = 6;
    int exclusiveZone = 44;
    int surfaceWidth = 600;
    int surfaceHeight = 420;
    // With a fullscreen window: "minimal" hides ambient activities, "hide"
    // hides everything but modals and critical notifications, "show" changes
    // nothing.
    std::string fullscreen = "minimal";
};

struct ThemeConfig {
    std::string source = "static"; // static | pywal
    std::string style = "black";   // black | blur
    std::string accent = "#0a84ff";
    std::string pywalAccent = "color4";
    double blurOpacity = 0.6;
};

struct ClockConfig {
    std::string format = "HH:mm";
};

struct NotificationRule {
    std::string app;     // matched against app name and desktop entry; * wildcards
    bool mute = false;   // drop entirely
    bool noPopup = false; // history only
    int urgency = -1;    // override: 0 low, 1 normal, 2 critical; -1 keep
};

struct NotificationsConfig {
    int timeout = 5000; // ms a popup stays when the app does not say
    int historyLimit = 100;
    std::vector<NotificationRule> rules;
};

struct LauncherConfig {
    std::string terminal;      // empty: $TERMINAL, then kitty, alacritty, foot
    std::string launchPrefix;  // e.g. "uwsm app --"
    std::string searchUrl = "https://duckduckgo.com/?q=%s"; // ?query in the launcher
};

struct CalendarConfig {
    // vdir collections (as written by vdirsyncer) or folders of .ics files.
    std::vector<std::string> paths = {"~/.calendars", "~/.local/share/calendars"};
    int days = 60;               // how far ahead to expand events
    int reminderMinutes = 10;    // show an upcoming event this long before
};

struct PowerConfig {
    std::string lockCommand = "hyprlock";
    int holdMs = 800; // press-and-hold time for shutdown and reboot
};

struct ConfigData {
    IslandConfig island;
    AnimationMode animations = AnimationMode::Full;
    ThemeConfig theme;
    ClockConfig clock;
    NotificationsConfig notifications;
    LauncherConfig launcher;
    PowerConfig power;
    CalendarConfig calendar;
    std::string iconTheme; // empty: platform default
    toml::table raw; // the whole document, for activity-specific settings
};

// Parses config.toml. Never throws: syntax errors and invalid values are
// appended to `errors` and the affected settings keep their defaults.
ConfigData parseConfig(std::string_view text, std::vector<std::string>& errors);

} // namespace island
