#pragma once
#include "../src/IDriveMotor.hpp"
#include <vector>
#include <string>
#include <iostream>

class SimDriveMotor : public IDriveMotor {
public:
    struct Call {
        enum Type { FORWARD, BACKWARD, STOP, TURN, ROTATE_RIGHT, ROTATE_LEFT } type;
        Direction dir{Direction::LEFT};
    };

    explicit SimDriveMotor(bool verbose = false) : verbose_(verbose) {}

    void moveForward() override {
        calls.push_back({Call::FORWARD});
        ++forwardCount;
        if (shared_) shared_->push_back("drive:forward");
        if (verbose_) log("DriveMotor", "moveForward()     ", "\033[32m");
    }
    void moveBackward() override {
        calls.push_back({Call::BACKWARD});
        ++backwardCount;
        if (shared_) shared_->push_back("drive:backward");
        if (verbose_) log("DriveMotor", "moveBackward()    ", "\033[33m");
    }
    void stop() override {
        calls.push_back({Call::STOP});
        ++stopCount;
        if (shared_) shared_->push_back("drive:stop");
        if (verbose_) log("DriveMotor", "stop()            ", "\033[31m");
    }
    void turn(Direction d) override {
        calls.push_back({Call::TURN, d});
        turnCalls.push_back(d);
        if (shared_) shared_->push_back(d == Direction::LEFT ? "drive:turn(L)" : "drive:turn(R)");
        if (verbose_) {
            std::string s = "turn("; s += (d == Direction::LEFT ? "LEFT" : "RIGHT"); s += ")";
            log("DriveMotor", s + "          ", "\033[36m");
        }
    }
    void rotateRight() override {
        calls.push_back({Call::ROTATE_RIGHT});
        ++rotateRightCount;
        if (shared_) shared_->push_back("drive:rotateRight");
        if (verbose_) log("DriveMotor", "rotateRight()     ", "\033[36m");
    }
    void rotateLeft() override {
        calls.push_back({Call::ROTATE_LEFT});
        ++rotateLeftCount;
        if (shared_) shared_->push_back("drive:rotateLeft");
        if (verbose_) log("DriveMotor", "rotateLeft()      ", "\033[36m");
    }

    void setSharedLog(std::vector<std::string>* log) { shared_ = log; }

    void reset() {
        calls.clear(); turnCalls.clear();
        forwardCount = backwardCount = stopCount = rotateRightCount = rotateLeftCount = 0;
    }

    std::vector<Call>      calls;
    std::vector<Direction> turnCalls;
    int forwardCount     = 0;
    int backwardCount    = 0;
    int stopCount        = 0;
    int rotateRightCount = 0;
    int rotateLeftCount  = 0;

private:
    bool verbose_;
    std::vector<std::string>* shared_ = nullptr;

    static void log(const std::string& who, const std::string& msg, const char* clr) {
        std::cout << "  " << clr << "← [" << who << "] " << msg << "\033[0m\n";
    }
};
