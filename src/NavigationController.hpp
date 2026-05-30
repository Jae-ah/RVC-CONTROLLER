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
    IDriveMotor& motor_;
};
