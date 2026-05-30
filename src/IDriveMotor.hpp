#pragma once
#include "Direction.hpp"

class IDriveMotor {
public:
    IDriveMotor()                                        = default;
    virtual ~IDriveMotor()                               = default;
    IDriveMotor(const IDriveMotor&)                      = delete;
    IDriveMotor& operator=(const IDriveMotor&) = delete;
    IDriveMotor(IDriveMotor&&)                 = delete;
    IDriveMotor& operator=(IDriveMotor&&)      = delete;
    virtual void moveForward() = 0;
    virtual void moveBackward() = 0;
    virtual void stop() = 0;
    virtual void turn(Direction direction) = 0;
    virtual void rotateRight() = 0;
    virtual void rotateLeft() = 0;
};
