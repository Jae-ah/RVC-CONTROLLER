#pragma once
#include "../src/RVCController.hpp"
#include "../src/NavigationController.hpp"
#include "../src/CleaningController.hpp"
#include "SimDriveMotor.hpp"
#include "SimCleaningMotor.hpp"
#include <iostream>
#include <string>
#include <vector>

// RVCController + 시뮬레이터 모터를 묶는 파사드.
// verbose=true  → 터미널 컬러 출력 (시각적 시뮬레이터 모드)
// verbose=false → 녹화만 (시스템 테스트 모드)
class RVCSimulator {
public:
    explicit RVCSimulator(bool verbose = false, int timerMs = 30000)
        : verbose_(verbose),
          drive_(verbose), cleaning_(verbose),
          nav_(drive_), cleanCtrl_(cleaning_, timerMs),
          rvc_(nav_, cleanCtrl_)
    {
        drive_.setSharedLog(&eventLog_);
        cleaning_.setSharedLog(&eventLog_);
    }

    // ── 시스템 오퍼레이션 ─────────────────────────────────────────
    void start() {
        emit("User         ", "start()                ", "\033[1;37m");
        rvc_.start();
    }
    void frontObstacleDetected() {
        emit("ObstacleSensor", "frontObstacleDetected()", "\033[1;33m");
        rvc_.frontObstacleDetected();
    }
    void sideStatus(const std::vector<Direction>& dirs) {
        emit("ObstacleSensor", "sideStatus(" + fmtDirs(dirs) + ")", "\033[1;33m");
        rvc_.sideStatus(dirs);
    }
    void allSidesBlocked() {
        emit("ObstacleSensor", "allSidesBlocked()      ", "\033[1;33m");
        rvc_.allSidesBlocked();
    }
    void dustDetected() {
        emit("DustSensor   ", "dustDetected()         ", "\033[1;35m");
        rvc_.dustDetected();
    }

    // ── 검증용 접근자 ──────────────────────────────────────────────
    SimDriveMotor&           drive()     { return drive_; }
    SimCleaningMotor&        cleaning()  { return cleaning_; }
    const std::vector<std::string>& eventLog() const { return eventLog_; }

    void resetRecording() {
        drive_.reset();
        cleaning_.reset();
        eventLog_.clear();
    }

private:
    bool verbose_;
    SimDriveMotor    drive_;
    SimCleaningMotor cleaning_;
    NavigationController nav_;
    CleaningController   cleanCtrl_;
    RVCController        rvc_;
    std::vector<std::string> eventLog_;   // 모터 호출 순서 기록

    void emit(const std::string& actor, const std::string& msg, const char* clr) const {
        if (!verbose_) return;
        std::cout << clr << "▶ [" << actor << "] " << msg << "\033[0m\n";
    }

    static std::string fmtDirs(const std::vector<Direction>& dirs) {
        if (dirs.empty()) return "{}";
        std::string s = "{";
        for (size_t i = 0; i < dirs.size(); ++i) {
            s += (dirs[i] == Direction::LEFT ? "LEFT" : "RIGHT");
            if (i + 1 < dirs.size()) s += ", ";
        }
        return s + "}";
    }
};
