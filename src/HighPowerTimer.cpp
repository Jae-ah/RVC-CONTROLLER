#include "HighPowerTimer.hpp"
#include "ITimerExpiredCallback.hpp"

HighPowerTimer::HighPowerTimer(ITimerExpiredCallback& callback, int durationMs)
    : callback_(callback), duration_(durationMs), active_(false) {}

HighPowerTimer::~HighPowerTimer() {
    stop();
}

void HighPowerTimer::start() {
    // 이전 타이머가 자연 만료된 경우 스레드가 아직 join되지 않았을 수 있다.
    // 재대입 전에 join해야 std::terminate를 방지할 수 있다.
    if (timerThread_.joinable()) {
        timerThread_.join();
    }
    active_ = true;
    timerThread_ = std::thread(&HighPowerTimer::run, this);
}

void HighPowerTimer::reset() {
    stop();
    start();
}

void HighPowerTimer::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        active_ = false;
    }
    cv_.notify_all();
    if (timerThread_.joinable()) {
        timerThread_.join();
    }
}

void HighPowerTimer::run() {
    std::unique_lock<std::mutex> lock(mutex_);
    bool timedOut = !cv_.wait_for(lock,
                                   std::chrono::milliseconds(duration_),
                                   [this] { return !active_.load(); });
    if (timedOut && active_) {
        active_ = false;
        lock.unlock();
        callback_.onExpired();
    }
}
