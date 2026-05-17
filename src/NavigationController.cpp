#include "NavigationController.hpp"
#include "IDriveMotor.hpp"
#include <random>
#include <stdexcept>

NavigationController::NavigationController(IDriveMotor& motor)
    : motor_(motor) {}

void NavigationController::moveForward() {
    motor_.moveForward();
}

void NavigationController::moveBackward() {
    motor_.moveBackward();
}

void NavigationController::stop() {
    motor_.stop();
}

void NavigationController::turn(const std::vector<Direction>& available) {
    if (available.empty()) {
        return;
    }

    Direction direction = available[0];
    if (available.size() != 1) {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<std::size_t> dist(0, available.size() - 1);
        direction = available[dist(rng)];
    }
    motor_.turn(direction);
}
