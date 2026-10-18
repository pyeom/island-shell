#pragma once

#include <QStringView>

#include <optional>

namespace island {

// The volume after `islandctl volume ACTION`: "up" and "down" step by 5%,
// "+N" and "-N" by N points, "N" sets N%. Nothing if ACTION is none of
// those. Steps stop at 100% (or where the volume already is, if above);
// a set value goes up to 150%, like the quick settings slider.
std::optional<double> volumeTarget(QStringView action, double current);

} // namespace island
