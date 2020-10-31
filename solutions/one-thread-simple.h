//
// Created by r00533983 on 10/31/2020.
//

#ifndef SPREADSHEETENGINE_ONE_THREAD_SIMPLE_H
#define SPREADSHEETENGINE_ONE_THREAD_SIMPLE_H

#include "solution.h"

class OneThreadSimpleSolution: public Solution {
private:

    struct CellInfo {
        std::vector<std::string> dependencies;
        bool is_calculated = false;
        ValueType value;
        Formula formula;
    };

    std::unordered_map<std::string, CellInfo> cells;

    void calculate(const std::string& cell);
    inline void recalculate_dependencies(const std::string& cell);

public:
    void InitialCalculate(const InputData& initial_data) override;
    void ChangeCell(const std::string& cell, const Formula& formula) override;
    OutputData GetCurrentValues() override;
};

#endif //SPREADSHEETENGINE_ONE_THREAD_SIMPLE_H
