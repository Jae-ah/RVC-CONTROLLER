#pragma once
#include "CleaningLevel.hpp"

class ICleaningMotor {
public:
    virtual ~ICleaningMotor() = default;
    virtual void startCleaning(CleaningLevel level) = 0;
    virtual void stopCleaning() = 0;
};
