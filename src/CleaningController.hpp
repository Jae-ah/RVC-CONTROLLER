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

private:
    ICleaningMotor& motor_;
    HighPowerTimer timer_;
    bool boosting_;
};
