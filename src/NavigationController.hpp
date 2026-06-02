#pragma once
#include "Direction.hpp"

class IDriveMotor;

class NavigationController {
public:
    explicit NavigationController(IDriveMotor& motor);

    void moveForward();
    void moveBackward();
    void stop();
    void turn(Direction direction);
    void rotateRight();
    void rotateLeft();
private:
    enum class MotorState {
        STOPPED, FORWARD, BACKWARD, LEFT, RIGHT, ROTATING_RIGHT, ROTATING_LEFT
    };

    IDriveMotor& motor_;
    MotorState motorState_ = MotorState::STOPPED;
};
