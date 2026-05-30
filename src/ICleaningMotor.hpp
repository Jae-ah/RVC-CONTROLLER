#pragma once
#include "CleaningLevel.hpp"

class ICleaningMotor {
public:
    ICleaningMotor()                                        = default;
    virtual ~ICleaningMotor()                               = default;
    ICleaningMotor(const ICleaningMotor&)                   = delete;
    ICleaningMotor& operator=(const ICleaningMotor&) = delete;
    ICleaningMotor(ICleaningMotor&&)                 = delete;
    ICleaningMotor& operator=(ICleaningMotor&&)      = delete;
    virtual void startCleaning(CleaningLevel level) = 0;
    virtual void stopCleaning() = 0;
};
