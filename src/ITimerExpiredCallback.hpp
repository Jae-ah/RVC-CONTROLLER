#pragma once

class ITimerExpiredCallback {
public:
    virtual ~ITimerExpiredCallback() = default;
    virtual void onExpired() = 0;
};
