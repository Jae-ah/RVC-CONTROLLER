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
    controller.stop();

    EXPECT_EQ(motor.stopCount, 1);
}

// turn() — 빈 목록이면 모터를 호출하지 않음
TEST_F(NavigationControllerTest, turn_emptyList_doesNotCallMotor) {
    controller.turn({});

    EXPECT_TRUE(motor.turnCalls.empty());
}

// turn() — 방향이 하나이면 해당 방향으로만 회전
TEST_F(NavigationControllerTest, turn_singleLeft_turnsLeft) {
    controller.turn({Direction::LEFT});

    ASSERT_EQ(motor.turnCalls.size(), 1u);
    EXPECT_EQ(motor.turnCalls[0], Direction::LEFT);
}

TEST_F(NavigationControllerTest, turn_singleRight_turnsRight) {
    controller.turn({Direction::RIGHT});

    ASSERT_EQ(motor.turnCalls.size(), 1u);
    EXPECT_EQ(motor.turnCalls[0], Direction::RIGHT);
}

// turn() — 방향이 여러 개이면 그 중 하나를 선택해 회전
TEST_F(NavigationControllerTest, turn_multipleDirections_picksExactlyOne) {
    controller.turn({Direction::LEFT, Direction::RIGHT});

    ASSERT_EQ(motor.turnCalls.size(), 1u);
    const Direction chosen = motor.turnCalls[0];
    EXPECT_TRUE(chosen == Direction::LEFT || chosen == Direction::RIGHT);
}

// turn() — 호출할 때마다 모터에 정확히 한 번만 전달
TEST_F(NavigationControllerTest, turn_calledTwice_motorCalledTwice) {
    controller.turn({Direction::LEFT});
    controller.turn({Direction::RIGHT});

    EXPECT_EQ(motor.turnCalls.size(), 2u);
}
