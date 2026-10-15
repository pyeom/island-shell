#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

namespace island {

// Damped harmonic oscillator with unit mass, integrated with semi-implicit Euler
// in fixed sub-steps so the result does not depend on the display refresh rate.
//
// Parameters follow SwiftUI's spring(response:dampingFraction:):
//   response        — period of the undamped oscillation, in seconds.
//   dampingFraction — 1.0 is critically damped, < 1 overshoots.
struct SpringModel {
    double value = 0.0;
    double velocity = 0.0;
    double target = 0.0;
    double stiffness = 0.0;
    double damping = 0.0;

    static constexpr double kSubStep = 1.0 / 480.0;
    static constexpr double kMaxFrame = 1.0 / 20.0;

    SpringModel() { setResponse(0.45, 0.8); }

    void setResponse(double response, double dampingFraction) {
        response = std::max(response, 0.01);
        dampingFraction = std::max(dampingFraction, 0.0);
        const double omega = 2.0 * std::numbers::pi / response;
        stiffness = omega * omega;
        damping = 2.0 * dampingFraction * omega;
    }

    // Advances the simulation by dt seconds. Long stalls are clamped so a
    // missed frame does not turn into a jump.
    void step(double dt) {
        dt = std::clamp(dt, 0.0, kMaxFrame);
        while (dt > 0.0) {
            const double h = std::min(kSubStep, dt);
            const double accel = -stiffness * (value - target) - damping * velocity;
            velocity += accel * h;
            value += velocity * h;
            dt -= h;
        }
    }

    [[nodiscard]] bool settled(double epsilon = 0.01) const {
        return std::abs(value - target) < epsilon && std::abs(velocity) < epsilon * 10.0;
    }

    void snap() {
        value = target;
        velocity = 0.0;
    }
};

} // namespace island
