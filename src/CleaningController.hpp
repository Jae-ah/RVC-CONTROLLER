#pragma once
#include "HighPowerTimer.hpp"
#include "ITimerExpiredCallback.hpp"

class ICleaningMotor;

class CleaningController : public ITimerExpiredCallback {
public:
    explicit CleaningController(ICleaningMotor& motor, int timerDurationMs = 3000);

    void startCleaning();
    void stopCleaning();
    void boostCleaning();
    void onExpired() override;

    void normalizePower();    // BOOST → NORMAL (중복 무시, OFF 시 거부)

private:
    ICleaningMotor& motor_;
    HighPowerTimer timer_;
    bool active_;
    bool boosting_;
};
