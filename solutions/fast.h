#ifndef SPREADSHEETENGINE_FAST_H
#define SPREADSHEETENGINE_FAST_H

#include <mutex>
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <concurrent_queue.h>

#include "solution.h"

class FastSolution : public Solution {
private:
    using Edges = Concurrency::concurrent_vector<std::string>;
    struct CellInfo {
        CellInfo() = default;
        CellInfo(const Formula& formula) : formula(formula), is_calculated(false) {}

        std::mutex mutex;
        std::atomic<ValueType> value;
        std::atomic<bool> is_calculated;
        Formula formula;
    };
    
    // Formula of these cells contains only numbers
    Concurrency::concurrent_vector<std::string> starting_cells;
    Concurrency::concurrent_unordered_map<std::string, Edges> DAG;
    Concurrency::concurrent_unordered_map<std::string, CellInfo*> cell_info;

    std::atomic<int> calculated_cells_count = 0;
    Concurrency::concurrent_queue<std::string> queue;

    void BuildDAG(const InputData& input_data);
    void ParallelBuildDAG(const InputData& input_data);
    void ParallelValuesCalculation();
    void ThreadJob();

public:
    void InitialCalculate(const InputData& input_data) override;
    void ChangeCell(const std::string& cell, const Formula& formula) override;
    OutputData GetCurrentValues() override;
};

#endif //SPREADSHEETENGINE_FAST_H
