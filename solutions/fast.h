#ifndef SPREADSHEETENGINE_FAST_H
#define SPREADSHEETENGINE_FAST_H

#include "solution.h"

class FastSolution: public Solution {
public:
    void InitialCalculate(const InputData& input_data) override;
    void ChangeCell(const std::string& cell, const Formula& formula) override;
    OutputData GetCurrentValues() override;
};

#endif //SPREADSHEETENGINE_FAST_H
