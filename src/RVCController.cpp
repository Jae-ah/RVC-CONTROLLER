#include "RVCController.hpp"
#include "CleaningController.hpp"
#include "NavigationController.hpp"

RVCController::RVCController(NavigationController& nav, CleaningController& cleaning)
    : nav_(nav), cleaning_(cleaning), state_(State::IDLE) {}

void RVCController::start() {
    cleaning_.startCleaning();
    nav_.moveForward();
    state_ = State::CLEANING;
}

void RVCController::frontObstacleDetected() {
    cleaning_.stopCleaning();
    nav_.stop();
    state_ = State::STOPPED;
}

void RVCController::sideStatus(const std::vector<Direction>& available) {
    if (state_ == State::STOPPED) {
        // SD-002: turn → startCleaning → moveForward
        nav_.turn(available);
        cleaning_.startCleaning();
        nav_.moveForward();
        state_ = State::CLEANING;
    } else if (state_ == State::REVERSING && !available.empty()) {
        // SD-003: stop → turn → moveForward → startCleaning
        nav_.stop();
        nav_.turn(available);
        nav_.moveForward();
        cleaning_.startCleaning();
        state_ = State::CLEANING;
    }
}

void RVCController::allSidesBlocked() {
    cleaning_.stopCleaning();
    nav_.moveBackward();
    state_ = State::REVERSING;
}

void RVCController::dustDetected() {
    cleaning_.boostCleaning();
}
