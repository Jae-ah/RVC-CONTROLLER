#include "NavigationController.hpp"
#include "IDriveMotor.hpp"

NavigationController::NavigationController(IDriveMotor& motor)
    : motor_(motor) {}

void NavigationController::moveForward() {
    if (motorState_ == MotorState::FORWARD) return;
    motor_.moveForward();
    motorState_ = MotorState::FORWARD;
}

void NavigationController::moveBackward() {
    if (motorState_ == MotorState::BACKWARD) return;
    motor_.moveBackward();
    motorState_ = MotorState::BACKWARD;
}

void NavigationController::stop() {
    if (motorState_ == MotorState::STOPPED) return;
    motor_.stop();
    motorState_ = MotorState::STOPPED;
}

void NavigationController::turn(Direction direction) {
    MotorState target = (direction == Direction::LEFT) ? MotorState::LEFT : MotorState::RIGHT;
    if (motorState_ == target) return;
    if (motorState_ == MotorState::FORWARD || motorState_ == MotorState::BACKWARD) return;
    motor_.turn(direction);
    motorState_ = target;
}

void NavigationController::rotateRight() {
    motor_.rotateRight();
    motorState_ = MotorState::ROTATING_RIGHT;
}

void NavigationController::rotateLeft() {
    motor_.rotateLeft();
    motorState_ = MotorState::ROTATING_LEFT;
}

