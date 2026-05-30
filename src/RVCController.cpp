#include "RVCController.hpp"
#include "CleaningController.hpp"
#include "NavigationController.hpp"

RVCController::RVCController(NavigationController& nav, CleaningController& cleaning)
    : nav_(nav), cleaning_(cleaning), state_(State::IDLE), leftClear_(false) {}

void RVCController::start() {
    cleaning_.startCleaning();
    nav_.moveForward();
    state_ = State::CLEANING;
}

void RVCController::frontObstacleDetected() {
    cleaning_.stopCleaning();
    nav_.stop();
    state_ = State::STOPPED;
}

void RVCController::sideStatus(Direction /*direction*/, bool clear) {
    switch (state_) {
        case State::STOPPED:
            // SD-002: sideStatus(LEFT) 수신 — rotateRight로 우측 스캔 준비
            leftClear_ = clear;
            nav_.rotateRight();
            state_ = State::STOPPED_CHECKING_RIGHT;
            break;

        case State::STOPPED_CHECKING_RIGHT:
            // SD-002: sideStatus(RIGHT) 수신 — rotateLeft로 정면 복귀 후 방향 결정
            nav_.rotateLeft();
            if (leftClear_) {
                nav_.turn(Direction::LEFT);
                cleaning_.startCleaning();
                nav_.moveForward();
                state_ = State::CLEANING;
            } else if (clear) {
                nav_.turn(Direction::RIGHT);
                cleaning_.startCleaning();
                nav_.moveForward();
                state_ = State::CLEANING;
            } else {
                // E1: 좌·우 모두 막힘 → UC-003
                nav_.moveBackward();
                state_ = State::REVERSING;
            }
            break;

        case State::REVERSING:
            // SD-003 루프: sideStatus(LEFT) 수신
            if (clear) {
                nav_.stop();
                nav_.turn(Direction::LEFT);
                nav_.moveForward();
                cleaning_.startCleaning();
                state_ = State::CLEANING;
            } else {
                nav_.rotateRight();
                state_ = State::REVERSING_CHECKING_RIGHT;
            }
            break;

        case State::REVERSING_CHECKING_RIGHT:
            // SD-003 루프: sideStatus(RIGHT) 수신 — rotateLeft로 정면 복귀
            nav_.rotateLeft();
            if (clear) {
                nav_.stop();
                nav_.turn(Direction::RIGHT);
                nav_.moveForward();
                cleaning_.startCleaning();
                state_ = State::CLEANING;
            } else {
                state_ = State::REVERSING;
            }
            break;

        default:
            break;
    }
}

void RVCController::dustDetected() {
    cleaning_.boostCleaning();
}
