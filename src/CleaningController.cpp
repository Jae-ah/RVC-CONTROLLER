#include "CleaningController.hpp"
#include "ICleaningMotor.hpp"
#include "CleaningLevel.hpp"

CleaningController::CleaningController(ICleaningMotor& motor, int timerDurationMs)
    : motor_(motor), timer_(*this, timerDurationMs), active_(false), boosting_(false) {}

void CleaningController::startCleaning() {
    if (active_) return;
    active_ = true;
    timer_.stop();
    boosting_ = false;
    motor_.startCleaning(CleaningLevel::NORMAL);
}

void CleaningController::stopCleaning() {
    if (!active_) return;
    active_ = false;
    timer_.stop();
    boosting_ = false;
    motor_.stopCleaning();
}

void CleaningController::boostCleaning() {
    if (!active_) return;
    if (boosting_) {
        timer_.reset();
    } else {
        boosting_ = true;
        motor_.startCleaning(CleaningLevel::HIGH);
        timer_.start();
    }
}

void CleaningController::onExpired() {
    boosting_ = false;
    motor_.startCleaning(CleaningLevel::NORMAL);
}

void CleaningController::normalizePower() {
    if (!active_) return;
    if (!boosting_) return;
    timer_.stop();
    boosting_ = false;
    motor_.startCleaning(CleaningLevel::NORMAL);
}
