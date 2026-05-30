#pragma once
#include "../src/IDriveMotor.hpp"
#include <vector>

class StubDriveMotor : public IDriveMotor {
public:
    void moveForward()  override { ++forwardCount; }
    void moveBackward() override { ++backwardCount; }
    void stop()         override { ++stopCount; }
    void turn(Direction dir) override { turnCalls.push_back(dir); }
    void rotateRight()  override { ++rotateRightCount; }
    void rotateLeft()   override { ++rotateLeftCount; }

    int forwardCount     = 0;
    int backwardCount    = 0;
    int stopCount        = 0;
    int rotateRightCount = 0;
    int rotateLeftCount  = 0;
    std::vector<Direction> turnCalls;
};
