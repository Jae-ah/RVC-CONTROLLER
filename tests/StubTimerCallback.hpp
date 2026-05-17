#pragma once
#include "../src/ITimerExpiredCallback.hpp"

class StubTimerCallback : public ITimerExpiredCallback {
public:
    void onExpired() override { ++callCount; }

    int callCount = 0;
};
