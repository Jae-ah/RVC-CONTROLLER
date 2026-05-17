#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "StubTimerCallback.hpp"
#include "../src/HighPowerTimer.hpp"

// 실시간 스레드를 사용하므로 짧은 duration(ms)으로 테스트한다.
// 각 케이스는 여유 시간(margin)을 두어 타이밍 오차를 흡수한다.

TEST(HighPowerTimerTest, start_notCalled_callbackNeverFires) {
    StubTimerCallback cb;
    { HighPowerTimer timer(cb, 50); } // start() 미호출 → 소멸자만 실행
    EXPECT_EQ(cb.callCount, 0);
}

TEST(HighPowerTimerTest, start_firesCallbackAfterDuration) {
    StubTimerCallback cb;
    HighPowerTimer timer(cb, 50);

    timer.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(120)); // 여유 margin

    EXPECT_EQ(cb.callCount, 1);
}

// 소멸자가 타이머를 정지시키므로 만료 전 파괴하면 콜백이 호출되지 않아야 한다.
TEST(HighPowerTimerTest, destructor_beforeExpiry_doesNotFireCallback) {
    StubTimerCallback cb;
    {
        HighPowerTimer timer(cb, 200);
        timer.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(30)); // 만료 전 탈출
    } // ← stop() 호출됨
    EXPECT_EQ(cb.callCount, 0);
}

// 만료 후 소멸해도 콜백은 정확히 한 번만 호출
TEST(HighPowerTimerTest, start_callbackFiredExactlyOnce) {
    StubTimerCallback cb;
    {
        HighPowerTimer timer(cb, 50);
        timer.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
    EXPECT_EQ(cb.callCount, 1);
}

// reset()은 타이머를 재시작한다 — 이전 카운트다운이 취소되고 새로 시작
TEST(HighPowerTimerTest, reset_restartsCountdown) {
    StubTimerCallback cb;
    HighPowerTimer timer(cb, 100);

    timer.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(60)); // 만료 전
    timer.reset(); // 100ms 카운트다운 재시작

    // reset 후 60ms 시점: 총 120ms 경과, 아직 만료되지 않아야 함
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    EXPECT_EQ(cb.callCount, 0);

    // reset 후 110ms 시점: 만료 완료
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    EXPECT_EQ(cb.callCount, 1);
}
