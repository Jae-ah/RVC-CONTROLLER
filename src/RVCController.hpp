#pragma once
#include "Direction.hpp"

class NavigationController;
class CleaningController;

class RVCController {
public:
    RVCController(NavigationController& nav, CleaningController& cleaning);

    void start();
    void frontObstacleDetected();
    void sideStatus(Direction direction, bool clear);
    void dustDetected();
    void dustDetected(float dustLevel);
    void changeState(int newState);

private:
    enum class State {
        IDLE,
        CLEANING,
        STOPPED,
        STOPPED_CHECKING_RIGHT,
        REVERSING,
        REVERSING_CHECKING_RIGHT
    };

    NavigationController& nav_;
    CleaningController& cleaning_;
    State state_;
    bool leftClear_;
};
