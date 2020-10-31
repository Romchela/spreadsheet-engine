#ifndef SPREADSHEETENGINE_SOLUTION_H
#define SPREADSHEETENGINE_SOLUTION_H

#include "../io-data.h"

class Solution {
public:
    virtual void ChangeCell(const std::string& cell, Formula formula) = 0;
    virtual OutputData GetCurrentValues() = 0;
    virtual void InitialCalculate(const InputData& inputData) = 0;
};

#endif //SPREADSHEETENGINE_SOLUTION_H
