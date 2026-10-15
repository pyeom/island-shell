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

struct ConfigData {
    IslandConfig island;
    AnimationMode animations = AnimationMode::Full;
    ThemeConfig theme;
    ClockConfig clock;
    toml::table raw; // the whole document, for activity-specific settings
};

// Parses config.toml. Never throws: syntax errors and invalid values are
// appended to `errors` and the affected settings keep their defaults.
ConfigData parseConfig(std::string_view text, std::vector<std::string>& errors);

} // namespace island
