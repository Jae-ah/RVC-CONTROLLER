#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "StubCleaningMotor.hpp"
#include "../src/CleaningController.hpp"

// 기본 픽스처 — 타이머 duration 기본값(3000ms) 사용
class CleaningControllerTest : public ::testing::Test {
protected:
    StubCleaningMotor motor;
    CleaningController controller{motor};
};

// 짧은 타이머 픽스처 — 실제 만료 흐름을 테스트할 때 사용
class CleaningControllerTimerTest : public ::testing::Test {
protected:
    StubCleaningMotor motor;
    CleaningController controller{motor, 80}; // 80ms
};

// ── startCleaning() ──────────────────────────────────────────────────────────

TEST_F(CleaningControllerTest, startCleaning_callsMotorWithNormalLevel) {
    controller.startCleaning();

    ASSERT_EQ(motor.startCalls.size(), 1u);
    EXPECT_EQ(motor.startCalls[0], CleaningLevel::NORMAL);
}

// ── stopCleaning() ───────────────────────────────────────────────────────────

TEST_F(CleaningControllerTest, stopCleaning_callsMotorStop) {
    controller.stopCleaning();

    EXPECT_EQ(motor.stopCount, 1);
}

// ── boostCleaning() ──────────────────────────────────────────────────────────

// 처음 호출 시 HIGH 레벨로 전환
TEST_F(CleaningControllerTest, boostCleaning_whenIdle_switchesToHighLevel) {
    controller.boostCleaning();

    ASSERT_EQ(motor.startCalls.size(), 1u);
    EXPECT_EQ(motor.startCalls[0], CleaningLevel::HIGH);
}

// 이미 부스트 중일 때 모터 재호출 없이 타이머만 리셋
TEST_F(CleaningControllerTest, boostCleaning_whenAlreadyBoosting_doesNotCallMotorAgain) {
    controller.boostCleaning();
    motor.startCalls.clear();

    controller.boostCleaning(); // 타이머 리셋만, 모터 호출 없음

    EXPECT_TRUE(motor.startCalls.empty());
}

// ── onExpired() ──────────────────────────────────────────────────────────────

// 타이머 만료 시 NORMAL 레벨로 복귀
TEST_F(CleaningControllerTest, onExpired_restoresNormalLevel) {
    controller.boostCleaning();
    motor.startCalls.clear();

    controller.onExpired();

    ASSERT_EQ(motor.startCalls.size(), 1u);
    EXPECT_EQ(motor.startCalls[0], CleaningLevel::NORMAL);
}

// 타이머가 실제로 만료되면 NORMAL로 복귀하고,
// 이후 boostCleaning()을 다시 호출하면 HIGH로 전환된다
TEST_F(CleaningControllerTimerTest, timerExpiry_restoresNormal_andAllowsSubsequentBoost) {
    controller.boostCleaning(); // HIGH 시작 + 80ms 타이머 가동
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 만료 대기

    // 만료 후 NORMAL 복귀 확인
    ASSERT_GE(motor.startCalls.size(), 2u);
    EXPECT_EQ(motor.startCalls.back(), CleaningLevel::NORMAL);

    motor.startCalls.clear();

    // 재부스트 — 타이머 스레드가 완전히 종료된 뒤이므로 안전하게 시작됨
    controller.boostCleaning();

    ASSERT_EQ(motor.startCalls.size(), 1u);
    EXPECT_EQ(motor.startCalls[0], CleaningLevel::HIGH);
}
