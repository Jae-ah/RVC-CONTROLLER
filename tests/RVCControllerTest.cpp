#include <gtest/gtest.h>
#include "StubCleaningMotor.hpp"
#include "StubDriveMotor.hpp"
#include "../src/NavigationController.hpp"
#include "../src/CleaningController.hpp"
#include "../src/RVCController.hpp"

// RVCController는 NavigationController·CleaningController를 인터페이스 없이
// 직접 참조하므로, 두 컨트롤러를 Stub 모터와 함께 실제 인스턴스로 주입한다.
class RVCControllerTest : public ::testing::Test {
protected:
    StubDriveMotor    driveMotor;
    StubCleaningMotor cleaningMotor;
    NavigationController nav{driveMotor};
    CleaningController   cleaning{cleaningMotor};
    RVCController        rvc{nav, cleaning};

    void resetCounters() {
        driveMotor.forwardCount  = 0;
        driveMotor.backwardCount = 0;
        driveMotor.stopCount     = 0;
        driveMotor.turnCalls.clear();
        cleaningMotor.startCalls.clear();
        cleaningMotor.stopCount = 0;
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

// ── sideStatus() — STOPPED 상태 ──────────────────────────────────────────────

TEST_F(RVCControllerTest, sideStatus_whenStopped_turnsToAvailableDirection) {
    rvc.start();
    rvc.frontObstacleDetected();
    resetCounters();

    rvc.sideStatus({Direction::LEFT});

    ASSERT_EQ(driveMotor.turnCalls.size(), 1u);
    EXPECT_EQ(driveMotor.turnCalls[0], Direction::LEFT);
}

TEST_F(RVCControllerTest, sideStatus_whenStopped_restartsCleaning) {
    rvc.start();
    rvc.frontObstacleDetected();
    resetCounters();

    rvc.sideStatus({Direction::RIGHT});

    ASSERT_EQ(cleaningMotor.startCalls.size(), 1u);
    EXPECT_EQ(cleaningMotor.startCalls[0], CleaningLevel::NORMAL);
}

TEST_F(RVCControllerTest, sideStatus_whenStopped_movesForward) {
    rvc.start();
    rvc.frontObstacleDetected();
    resetCounters();

    rvc.sideStatus({Direction::LEFT});

    EXPECT_EQ(driveMotor.forwardCount, 1);
}

// ── sideStatus() — CLEANING 상태 (무시) ──────────────────────────────────────

TEST_F(RVCControllerTest, sideStatus_whenCleaning_isIgnored) {
    rvc.start();
    resetCounters();

    rvc.sideStatus({Direction::LEFT}); // CLEANING 상태이므로 아무 동작 없음

    EXPECT_TRUE(driveMotor.turnCalls.empty());
    EXPECT_TRUE(cleaningMotor.startCalls.empty());
    EXPECT_EQ(driveMotor.forwardCount, 0);
}

// ── allSidesBlocked() ────────────────────────────────────────────────────────

TEST_F(RVCControllerTest, allSidesBlocked_stopsCleaning) {
    rvc.start();
    resetCounters();

    rvc.allSidesBlocked();

    EXPECT_EQ(cleaningMotor.stopCount, 1);
}

TEST_F(RVCControllerTest, allSidesBlocked_movesBackward) {
    rvc.start();
    resetCounters();

    rvc.allSidesBlocked();

    EXPECT_EQ(driveMotor.backwardCount, 1);
}

// ── sideStatus() — REVERSING 상태 ────────────────────────────────────────────

TEST_F(RVCControllerTest, sideStatus_whenReversing_withAvailableDir_stopsMotor) {
    rvc.start();
    rvc.allSidesBlocked();
    resetCounters();

    rvc.sideStatus({Direction::RIGHT});

    EXPECT_EQ(driveMotor.stopCount, 1);
}

TEST_F(RVCControllerTest, sideStatus_whenReversing_withAvailableDir_turns) {
    rvc.start();
    rvc.allSidesBlocked();
    resetCounters();

    rvc.sideStatus({Direction::RIGHT});

    ASSERT_EQ(driveMotor.turnCalls.size(), 1u);
    EXPECT_EQ(driveMotor.turnCalls[0], Direction::RIGHT);
}

TEST_F(RVCControllerTest, sideStatus_whenReversing_withAvailableDir_movesForward) {
    rvc.start();
    rvc.allSidesBlocked();
    resetCounters();

    rvc.sideStatus({Direction::RIGHT});

    EXPECT_EQ(driveMotor.forwardCount, 1);
}

TEST_F(RVCControllerTest, sideStatus_whenReversing_withAvailableDir_restartsCleaning) {
    rvc.start();
    rvc.allSidesBlocked();
    resetCounters();

    rvc.sideStatus({Direction::LEFT});

    ASSERT_EQ(cleaningMotor.startCalls.size(), 1u);
    EXPECT_EQ(cleaningMotor.startCalls[0], CleaningLevel::NORMAL);
}

// REVERSING 상태에서 가용 방향이 없으면 그대로 후진 유지
TEST_F(RVCControllerTest, sideStatus_whenReversing_withEmptyDir_isIgnored) {
    rvc.start();
    rvc.allSidesBlocked();
    resetCounters();

    rvc.sideStatus({}); // 여전히 전방위 막힘 → 아무 동작 없음

    EXPECT_EQ(driveMotor.stopCount, 0);
    EXPECT_TRUE(driveMotor.turnCalls.empty());
    EXPECT_EQ(driveMotor.forwardCount, 0);
}

// ── dustDetected() ───────────────────────────────────────────────────────────

TEST_F(RVCControllerTest, dustDetected_boostsCleaningToHighLevel) {
    rvc.start();
    resetCounters();

    rvc.dustDetected();

    ASSERT_EQ(cleaningMotor.startCalls.size(), 1u);
    EXPECT_EQ(cleaningMotor.startCalls[0], CleaningLevel::HIGH);
}
