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
