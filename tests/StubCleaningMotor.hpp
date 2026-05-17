#pragma once
#include "../src/ICleaningMotor.hpp"
#include <vector>

class StubCleaningMotor : public ICleaningMotor {
public:
    void startCleaning(CleaningLevel level) override {
        startCalls.push_back(level);
    }
    void stopCleaning() override {
        ++stopCount;
    }

    std::vector<CleaningLevel> startCalls;
    int stopCount = 0;
};
