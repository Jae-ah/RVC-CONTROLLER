#pragma once

class ITimerExpiredCallback {
public:
    ITimerExpiredCallback()                                        = default;
    virtual ~ITimerExpiredCallback()                               = default;
    ITimerExpiredCallback(const ITimerExpiredCallback&)            = delete;
    ITimerExpiredCallback& operator=(const ITimerExpiredCallback&) = delete;
    ITimerExpiredCallback(ITimerExpiredCallback&&)                 = delete;
    ITimerExpiredCallback& operator=(ITimerExpiredCallback&&)      = delete;
    virtual void onExpired() = 0;
};
