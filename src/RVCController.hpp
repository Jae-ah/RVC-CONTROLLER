#pragma once
#include "Direction.hpp"
#include <vector>

class NavigationController;
class CleaningController;

class RVCController {
public:
    RVCController(NavigationController& nav, CleaningController& cleaning);

    void start();
    void frontObstacleDetected();
    void sideStatus(const std::vector<Direction>& available);
    void allSidesBlocked();
    void dustDetected();

private:
    enum class State { IDLE, CLEANING, STOPPED, REVERSING };

    NavigationController& nav_;
    CleaningController& cleaning_;
    State state_;
};
