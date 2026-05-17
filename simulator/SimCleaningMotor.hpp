#pragma once
#include "../src/ICleaningMotor.hpp"
#include <vector>
#include <string>
#include <iostream>

class SimCleaningMotor : public ICleaningMotor {
public:
    explicit SimCleaningMotor(bool verbose = false) : verbose_(verbose) {}

    void startCleaning(CleaningLevel level) override {
        startCalls.push_back(level);
        if (shared_) {
            shared_->push_back(level == CleaningLevel::HIGH
                               ? "clean:start(HIGH)" : "clean:start(NORMAL)");
        }
        if (verbose_) {
            const char* clr = (level == CleaningLevel::HIGH) ? "\033[35m" : "\033[34m";
            std::string s = "startCleaning(";
            s += (level == CleaningLevel::HIGH ? "HIGH" : "NORMAL"); s += ")";
            log("CleaningMotor", s, clr);
        }
    }
    void stopCleaning() override {
        ++stopCount;
        if (shared_) shared_->push_back("clean:stop");
        if (verbose_) log("CleaningMotor", "stopCleaning()    ", "\033[31m");
    }

    void setSharedLog(std::vector<std::string>* log) { shared_ = log; }

    void reset() { startCalls.clear(); stopCount = 0; }

    std::vector<CleaningLevel> startCalls;
    int stopCount = 0;

private:
    bool verbose_;
    std::vector<std::string>* shared_ = nullptr;

    static void log(const std::string& who, const std::string& msg, const char* clr) {
        std::cout << "  " << clr << "← [" << who << "] " << msg << "\033[0m\n";
    }
};
