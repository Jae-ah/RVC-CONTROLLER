#pragma once
#include "Direction.hpp"
#include <vector>

class IDriveMotor;

class NavigationController {
public:
    explicit NavigationController(IDriveMotor& motor);

    void moveForward();
    void moveBackward();
    void stop();
    void turn(const std::vector<Direction>& available);

private:
    IDriveMotor& motor_;
};
