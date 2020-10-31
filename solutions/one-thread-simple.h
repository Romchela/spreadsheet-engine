//
// Created by r00533983 on 10/31/2020.
//

#ifndef SPREADSHEETENGINE_ONE_THREAD_SIMPLE_H
#define SPREADSHEETENGINE_ONE_THREAD_SIMPLE_H

#include "solution.h"

class OneThreadSimpleSolution: public Solution {
private:


public:
    void InitialCalculate(const InputData& inputData) override;
    void ChangeCell(const std::string& cell, Formula formula) override;
    OutputData GetCurrentValues() override;
};

#endif //SPREADSHEETENGINE_ONE_THREAD_SIMPLE_H
