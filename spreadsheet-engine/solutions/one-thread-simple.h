#ifndef SPREADSHEETENGINE_ONE_THREAD_SIMPLE_H
#define SPREADSHEETENGINE_ONE_THREAD_SIMPLE_H

#include <unordered_set>
#include "solution.h"

// Solution is simple implementation of task in one thread.
// It based on dfs (depth-first search).
class OneThreadSimpleSolution: public Solution {
private:

    struct CellInfo {
        std::unordered_set<std::string> dependencies;
        bool is_calculated = false;
        ValueType value;
        Formula formula;
    };

    std::unordered_map<std::string, CellInfo> cells;

    std::unordered_set<std::string> need_to_recalculate;
    std::vector<std::string> top_sort_recalculations;

    void Calculate(const std::string& cell);
    void BuildTopSortRecalculations(const std::string& cell);
    void CalculateDependencies(const std::string& cell);
    void RecalculateDependencies(const std::string& cell, const Formula& formula);

public:

    // Time complexity is O(n) where n - number of vertices in input_data.
    void InitialCalculate(const InputData& initial_data) override;

    // Time complexity is O(t) where t - total number of cells which are depended on 'cell'.
    void ChangeCell(const std::string& cell, const Formula& formula) override;

    OutputData GetCurrentValues() override;
};

#endif //SPREADSHEETENGINE_ONE_THREAD_SIMPLE_H
