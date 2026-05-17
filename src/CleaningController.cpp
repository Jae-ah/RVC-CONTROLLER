#include "CleaningController.hpp"
#include "ICleaningMotor.hpp"
#include "CleaningLevel.hpp"

CleaningController::CleaningController(ICleaningMotor& motor, int timerDurationMs)
    : motor_(motor), timer_(*this, timerDurationMs), boosting_(false) {}

void CleaningController::startCleaning() {
    timer_.stop();      // 진행 중인 HIGH 타이머 취소
    boosting_ = false;  // boost 상태 해제 → 다음 dustDetected()가 HIGH 재활성화 가능
    motor_.startCleaning(CleaningLevel::NORMAL);
}

void CleaningController::stopCleaning() {
    timer_.stop();      // 진행 중인 HIGH 타이머 취소
    boosting_ = false;
    motor_.stopCleaning();
}

void CleaningController::boostCleaning() {
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
