#include "volumestep.h"

#include <algorithm>
#include <cmath>

namespace island {

std::optional<double> volumeTarget(QStringView action, double current) {
    constexpr double kStep = 0.05;
    constexpr double kMax = 1.5;
    double delta = 0;
    if (action == u"up") {
        delta = kStep;
    } else if (action == u"down") {
        delta = -kStep;
    } else {
        bool ok = false;
        const double n = action.toDouble(&ok);
        if (!ok || !std::isfinite(n))
            return std::nullopt;
        if (!action.startsWith(u'+') && !action.startsWith(u'-'))
            return std::clamp(n / 100, 0.0, kMax);
        delta = n / 100;
    }
    const double ceiling = delta > 0 ? std::max(1.0, current) : kMax;
    // Whole percents, so 43% + 5 is 48%, not 48.0001%.
    return std::clamp(std::round((current + delta) * 100) / 100, 0.0, ceiling);
}

} // namespace island
