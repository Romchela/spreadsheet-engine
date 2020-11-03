#ifndef SPREADSHEETENGINE_SOLUTION_H
#define SPREADSHEETENGINE_SOLUTION_H

#include <iostream>
#include <thread>
#include <chrono>

#include "../io-data.h"

const int milliseconds = 0;

inline ValueType sum(ValueType a, ValueType b) {    
    //std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    return a + b;
}

class Solution {
public:
    virtual void InitialCalculate(const InputData& inputData) = 0;
    virtual void ChangeCell(const std::string& cell, const Formula& formula) = 0;
    virtual OutputData GetCurrentValues() = 0;
};

#endif //SPREADSHEETENGINE_SOLUTION_H
