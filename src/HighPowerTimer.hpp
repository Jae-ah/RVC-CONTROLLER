#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class ITimerExpiredCallback;

class HighPowerTimer {
public:
    explicit HighPowerTimer(ITimerExpiredCallback& callback, int durationMs = 3000);
    ~HighPowerTimer();

    HighPowerTimer(const HighPowerTimer&)            = delete;
    HighPowerTimer& operator=(const HighPowerTimer&) = delete;
    HighPowerTimer(HighPowerTimer&&)                 = delete;
    HighPowerTimer& operator=(HighPowerTimer&&)      = delete;

    void start();
    void reset();
    void stop();    // 타이머 취소 (onExpired 호출 없이 즉시 중단)

private:
    ITimerExpiredCallback& callback_;
    int duration_;
    std::atomic<bool> active_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread timerThread_;

    void run();
};
