#include <gtest/gtest.h>
#include "StubCleaningMotor.hpp"
#include "StubDriveMotor.hpp"
#include "../src/NavigationController.hpp"
#include "../src/CleaningController.hpp"
#include "../src/RVCController.hpp"

class RVCControllerTest : public ::testing::Test {
protected:
    StubDriveMotor    driveMotor;
    StubCleaningMotor cleaningMotor;
    NavigationController nav{driveMotor};
    CleaningController   cleaning{cleaningMotor};
    RVCController        rvc{nav, cleaning};

    void resetCounters() {
        driveMotor.forwardCount     = 0;
        driveMotor.backwardCount    = 0;
        driveMotor.stopCount        = 0;
        driveMotor.rotateRightCount = 0;
        driveMotor.rotateLeftCount  = 0;
        driveMotor.turnCalls.clear();
        cleaningMotor.startCalls.clear();
        cleaningMotor.stopCount = 0;
    }

    // 전방 장애물 감지 후 좌우 sideStatus 두 번 호출 (SD-002 흐름)
    void obstacleAndSideCheck(bool leftClear, bool rightClear) {
        rvc.frontObstacleDetected();
        rvc.sideStatus(Direction::LEFT, leftClear);
        rvc.sideStatus(Direction::RIGHT, rightClear);
    }

    // 좌우 모두 막힘(E1)으로 REVERSING 진입
    void enterReversingState() {
        rvc.start();
        obstacleAndSideCheck(false, false);
        resetCounters();
    }
};

// ── start() ─────────────────────────────────────────────────────────────────

TEST_F(RVCControllerTest, start_startsCleaning_atNormalLevel) {
    rvc.start();

    ASSERT_EQ(cleaningMotor.startCalls.size(), 1u);
    EXPECT_EQ(cleaningMotor.startCalls[0], CleaningLevel::NORMAL);
}

TEST_F(RVCControllerTest, start_movesForward) {
    rvc.start();

    EXPECT_EQ(driveMotor.forwardCount, 1);
}

// ── frontObstacleDetected() ──────────────────────────────────────────────────

TEST_F(RVCControllerTest, frontObstacleDetected_stopsCleaning) {
    rvc.start();
    resetCounters();

    rvc.frontObstacleDetected();

    EXPECT_EQ(cleaningMotor.stopCount, 1);
}

TEST_F(RVCControllerTest, frontObstacleDetected_stopsMotor) {
    rvc.start();
    resetCounters();

    rvc.frontObstacleDetected();

    EXPECT_EQ(driveMotor.stopCount, 1);
}

// ── sideStatus() — STOPPED: 첫 번째 호출(LEFT)은 항상 rotateRight ──────────

TEST_F(RVCControllerTest, sideStatus_firstCall_rotatesRight) {
    rvc.start();
    rvc.frontObstacleDetected();
    resetCounters();

    rvc.sideStatus(Direction::LEFT, true);

    EXPECT_EQ(driveMotor.rotateRightCount, 1);
}

TEST_F(RVCControllerTest, sideStatus_secondCall_rotatesLeft) {
    rvc.start();
    rvc.frontObstacleDetected();
    rvc.sideStatus(Direction::LEFT, true);
    resetCounters();

    rvc.sideStatus(Direction::RIGHT, false);

    EXPECT_EQ(driveMotor.rotateLeftCount, 1);
}

// ── sideStatus() — STOPPED: 좌측 여유 → LEFT 우선 ──────────────────────────

TEST_F(RVCControllerTest, sideStatus_leftClear_turnsLeft) {
    rvc.start();
    resetCounters();

    obstacleAndSideCheck(/*leftClear=*/true, /*rightClear=*/false);

    ASSERT_EQ(driveMotor.turnCalls.size(), 1u);
    EXPECT_EQ(driveMotor.turnCalls[0], Direction::LEFT);
}

TEST_F(RVCControllerTest, sideStatus_leftClear_restartsCleaning) {
    rvc.start();
    resetCounters();

    obstacleAndSideCheck(true, false);

    ASSERT_EQ(cleaningMotor.startCalls.size(), 1u);
    EXPECT_EQ(cleaningMotor.startCalls[0], CleaningLevel::NORMAL);
}

TEST_F(RVCControllerTest, sideStatus_leftClear_movesForward) {
    rvc.start();
    resetCounters();

    obstacleAndSideCheck(true, false);

    EXPECT_EQ(driveMotor.forwardCount, 1);
}

// ── sideStatus() — STOPPED: 우측만 여유 ──────────────────────────────────────

TEST_F(RVCControllerTest, sideStatus_leftBlockedRightClear_turnsRight) {
    rvc.start();
    resetCounters();

    obstacleAndSideCheck(false, true);

    ASSERT_EQ(driveMotor.turnCalls.size(), 1u);
    EXPECT_EQ(driveMotor.turnCalls[0], Direction::RIGHT);
}

TEST_F(RVCControllerTest, sideStatus_leftBlockedRightClear_restartsCleaning) {
    rvc.start();
    resetCounters();

    obstacleAndSideCheck(false, true);

    ASSERT_EQ(cleaningMotor.startCalls.size(), 1u);
    EXPECT_EQ(cleaningMotor.startCalls[0], CleaningLevel::NORMAL);
}

TEST_F(RVCControllerTest, sideStatus_leftBlockedRightClear_movesForward) {
    rvc.start();
    resetCounters();

    obstacleAndSideCheck(false, true);

    EXPECT_EQ(driveMotor.forwardCount, 1);
}

// ── sideStatus() — E1: 좌·우 모두 막힘 → REVERSING ─────────────────────────

TEST_F(RVCControllerTest, sideStatus_bothBlocked_movesBackward) {
    rvc.start();
    resetCounters();

    obstacleAndSideCheck(false, false);

    EXPECT_EQ(driveMotor.backwardCount, 1);
}

TEST_F(RVCControllerTest, sideStatus_bothBlocked_doesNotTurn) {
    rvc.start();
    resetCounters();

    obstacleAndSideCheck(false, false);

    EXPECT_TRUE(driveMotor.turnCalls.empty());
}

// ── sideStatus() — CLEANING 상태 (무시) ──────────────────────────────────────

TEST_F(RVCControllerTest, sideStatus_whenCleaning_isIgnored) {
    rvc.start();
    resetCounters();

    rvc.sideStatus(Direction::LEFT, true);

    EXPECT_EQ(driveMotor.rotateRightCount, 0);
    EXPECT_TRUE(driveMotor.turnCalls.empty());
    EXPECT_EQ(driveMotor.forwardCount, 0);
}

// ── sideStatus() — REVERSING: 좌측 여유 ──────────────────────────────────────

TEST_F(RVCControllerTest, sideStatus_whenReversing_leftClear_stops) {
    enterReversingState();

    rvc.sideStatus(Direction::LEFT, true);

    EXPECT_EQ(driveMotor.stopCount, 1);
}

TEST_F(RVCControllerTest, sideStatus_whenReversing_leftClear_turnsLeft) {
    enterReversingState();

    rvc.sideStatus(Direction::LEFT, true);

    ASSERT_EQ(driveMotor.turnCalls.size(), 1u);
    EXPECT_EQ(driveMotor.turnCalls[0], Direction::LEFT);
}

TEST_F(RVCControllerTest, sideStatus_whenReversing_leftClear_movesForward) {
    enterReversingState();

    rvc.sideStatus(Direction::LEFT, true);

    EXPECT_EQ(driveMotor.forwardCount, 1);
}

TEST_F(RVCControllerTest, sideStatus_whenReversing_leftClear_restartsCleaning) {
    enterReversingState();

    rvc.sideStatus(Direction::LEFT, true);

    ASSERT_EQ(cleaningMotor.startCalls.size(), 1u);
    EXPECT_EQ(cleaningMotor.startCalls[0], CleaningLevel::NORMAL);
}

// ── sideStatus() — REVERSING: 좌측 막힘 → rotateRight 호출 ─────────────────

TEST_F(RVCControllerTest, sideStatus_whenReversing_leftBlocked_rotatesRight) {
    enterReversingState();

    rvc.sideStatus(Direction::LEFT, false);

    EXPECT_EQ(driveMotor.rotateRightCount, 1);
}

// ── sideStatus() — REVERSING: 우측만 여유 ────────────────────────────────────

TEST_F(RVCControllerTest, sideStatus_whenReversing_rightOnly_turnsRight) {
    enterReversingState();

    rvc.sideStatus(Direction::LEFT, false);
    rvc.sideStatus(Direction::RIGHT, true);

    ASSERT_EQ(driveMotor.turnCalls.size(), 1u);
    EXPECT_EQ(driveMotor.turnCalls[0], Direction::RIGHT);
}

TEST_F(RVCControllerTest, sideStatus_whenReversing_rightOnly_movesForward) {
    enterReversingState();

    rvc.sideStatus(Direction::LEFT, false);
    rvc.sideStatus(Direction::RIGHT, true);

    EXPECT_EQ(driveMotor.forwardCount, 1);
}

TEST_F(RVCControllerTest, sideStatus_whenReversing_rightOnly_restartsCleaning) {
    enterReversingState();

    rvc.sideStatus(Direction::LEFT, false);
    rvc.sideStatus(Direction::RIGHT, true);

    ASSERT_EQ(cleaningMotor.startCalls.size(), 1u);
    EXPECT_EQ(cleaningMotor.startCalls[0], CleaningLevel::NORMAL);
}

// ── sideStatus() — REVERSING: 좌·우 모두 막힘 (루프 유지) ───────────────────

TEST_F(RVCControllerTest, sideStatus_whenReversing_bothBlocked_noTurnOrForward) {
    enterReversingState();

    rvc.sideStatus(Direction::LEFT, false);
    rvc.sideStatus(Direction::RIGHT, false);

    EXPECT_TRUE(driveMotor.turnCalls.empty());
    EXPECT_EQ(driveMotor.forwardCount, 0);
}

TEST_F(RVCControllerTest, sideStatus_whenReversing_bothBlocked_rotatesRightThenLeft) {
    enterReversingState();
    resetCounters();

    rvc.sideStatus(Direction::LEFT, false);
    rvc.sideStatus(Direction::RIGHT, false);

    EXPECT_EQ(driveMotor.rotateRightCount, 1);
    EXPECT_EQ(driveMotor.rotateLeftCount, 1);
}

// ── dustDetected() ───────────────────────────────────────────────────────────

TEST_F(RVCControllerTest, dustDetected_boostsCleaningToHighLevel) {
    rvc.start();
    resetCounters();

    rvc.dustDetected();

    ASSERT_EQ(cleaningMotor.startCalls.size(), 1u);
    EXPECT_EQ(cleaningMotor.startCalls[0], CleaningLevel::HIGH);
}

// ── dustDetected(float) ──────────────────────────────────────────────────────

// 양수 dustLevel → HIGH 전환
TEST_F(RVCControllerTest, dustDetected_positiveLevel_boostsCleaning) {
    rvc.start();
    resetCounters();

    rvc.dustDetected(0.5f);

    ASSERT_EQ(cleaningMotor.startCalls.size(), 1u);
    EXPECT_EQ(cleaningMotor.startCalls[0], CleaningLevel::HIGH);
}

// 음수 dustLevel → 무시
TEST_F(RVCControllerTest, dustDetected_negativeLevel_isIgnored) {
    rvc.start();
    resetCounters();

    rvc.dustDetected(-1.0f);

    EXPECT_TRUE(cleaningMotor.startCalls.empty());
}

// ── changeState(int) ─────────────────────────────────────────────────────────

// 유효 전이: IDLE(0) → CLEANING(1) — start() 중복 호출이 무시되어야 함
TEST_F(RVCControllerTest, changeState_idleToCleaning_changesState) {
    rvc.changeState(1);        // state_ = CLEANING (active_는 false 유지)

    // CLEANING 상태이므로 start() 가 무시됨
    rvc.start();
    EXPECT_TRUE(cleaningMotor.startCalls.empty());
    EXPECT_EQ(driveMotor.forwardCount, 0);
}

// 유효 전이: CLEANING(1) → IDLE(0) — start()가 다시 동작해야 함
TEST_F(RVCControllerTest, changeState_cleaningToIdle_changesState) {
    rvc.changeState(1);        // state_ = CLEANING
    rvc.changeState(0);        // state_ = IDLE (active_는 여전히 false)

    rvc.start();               // IDLE이므로 start() 정상 동작
    ASSERT_EQ(cleaningMotor.startCalls.size(), 1u);
    EXPECT_EQ(cleaningMotor.startCalls[0], CleaningLevel::NORMAL);
}

// 범위 외 값 → 무시 (상태 변화 없음)
TEST_F(RVCControllerTest, changeState_outOfRange_isIgnored) {
    rvc.start();               // CLEANING 상태
    resetCounters();

    rvc.changeState(999);
    rvc.changeState(-1);

    // 여전히 CLEANING이므로 frontObstacleDetected 가 stopCleaning 호출
    rvc.frontObstacleDetected();
    EXPECT_EQ(cleaningMotor.stopCount, 1);
}

// 동일 상태 전이 → 무시
TEST_F(RVCControllerTest, changeState_sameState_isIgnored) {
    rvc.start();               // CLEANING 상태
    resetCounters();

    rvc.changeState(1);        // 이미 CLEANING — 무시

    // 모터 추가 호출 없음
    EXPECT_TRUE(cleaningMotor.startCalls.empty());
}
