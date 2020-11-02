#ifndef SPREADSHEETENGINE_SOLUTION_H
#define SPREADSHEETENGINE_SOLUTION_H

#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#else
#include <thread>
#include <chrono>
#endif

#include "../io-data.h"

inline ValueType sum(ValueType a, ValueType b) {
    const int milliseconds = 0;
#ifdef _WIN32
    Sleep(milliseconds);
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#endif
    return a + b;
}

class Solution {
public:
    virtual void ChangeCell(const std::string& cell, const Formula& formula) = 0;
    virtual OutputData GetCurrentValues() = 0;
    virtual void InitialCalculate(const InputData& inputData) = 0;
};

#endif //SPREADSHEETENGINE_SOLUTION_H
