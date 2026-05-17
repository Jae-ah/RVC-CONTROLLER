#pragma once
#include "Direction.hpp"

class IDriveMotor {
public:
    virtual ~IDriveMotor() = default;
    virtual void moveForward() = 0;
    virtual void moveBackward() = 0;
    virtual void stop() = 0;
    virtual void turn(Direction direction) = 0;
};
