#include "NavigationController.hpp"
#include "IDriveMotor.hpp"

NavigationController::NavigationController(IDriveMotor& motor)
    : motor_(motor) {}

void NavigationController::moveForward() {
    motor_.moveForward();
}

void NavigationController::moveBackward() {
    motor_.moveBackward();
}

void NavigationController::stop() {
    motor_.stop();
}

void NavigationController::turn(Direction direction) {
    motor_.turn(direction);
}

void NavigationController::rotateRight() {
    motor_.rotateRight();
}

void NavigationController::rotateLeft() {
    motor_.rotateLeft();
}
