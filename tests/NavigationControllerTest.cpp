#include <gtest/gtest.h>
#include "StubDriveMotor.hpp"
#include "../src/NavigationController.hpp"

class NavigationControllerTest : public ::testing::Test {
protected:
    StubDriveMotor motor;
    NavigationController controller{motor};
};

TEST_F(NavigationControllerTest, moveForward_delegatesToMotor) {
    controller.moveForward();

    EXPECT_EQ(motor.forwardCount, 1);
}

TEST_F(NavigationControllerTest, moveBackward_delegatesToMotor) {
    controller.moveBackward();

    EXPECT_EQ(motor.backwardCount, 1);
}

TEST_F(NavigationControllerTest, stop_delegatesToMotor) {
    controller.moveForward();  // STOPPED → FORWARD 선행 상태 설정
    controller.stop();

    EXPECT_EQ(motor.stopCount, 1);
}

TEST_F(NavigationControllerTest, turn_left_delegatesToMotor) {
    controller.turn(Direction::LEFT);

    ASSERT_EQ(motor.turnCalls.size(), 1u);
    EXPECT_EQ(motor.turnCalls[0], Direction::LEFT);
}

TEST_F(NavigationControllerTest, turn_right_delegatesToMotor) {
    controller.turn(Direction::RIGHT);

    ASSERT_EQ(motor.turnCalls.size(), 1u);
    EXPECT_EQ(motor.turnCalls[0], Direction::RIGHT);
}

TEST_F(NavigationControllerTest, rotateRight_delegatesToMotor) {
    controller.rotateRight();

    EXPECT_EQ(motor.rotateRightCount, 1);
}

TEST_F(NavigationControllerTest, rotateLeft_delegatesToMotor) {
    controller.rotateLeft();

    EXPECT_EQ(motor.rotateLeftCount, 1);
}
