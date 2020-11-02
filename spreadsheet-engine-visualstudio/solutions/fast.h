#ifndef SPREADSHEETENGINE_FAST_H
#define SPREADSHEETENGINE_FAST_H

#include <mutex>
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>
#include <concurrent_queue.h>

#include "solution.h"

// This solution based on bfs (breadth-first search) which is easy to parallelize.
// InitialCalculate() and ChangeCell() methods have the same time and space complexity as OneThreadSimple solution has,
// but FastSolution is much faster because we do work in multiple threads.
//
// We use collections from Concurrency namespace because they are thread-safe and efficient for parallel access/modifications.
// More details: https://docs.microsoft.com/ru-ru/cpp/parallel/concrt/parallel-containers-and-objects
class FastSolution : public Solution {
private:
    using Edges = Concurrency::concurrent_unordered_map<std::string, bool>;
    struct CellInfo {
        CellInfo() = default;
        CellInfo(const Formula& formula) : 
            formula(formula), is_calculated(false), unresolved_cells_count(0) {}

        std::mutex mutex;
        std::atomic<ValueType> value;
        std::atomic<bool> is_calculated;
        Formula formula;
        std::atomic<int> unresolved_cells_count;
    };
    
    std::atomic<int> recalculation_count = 0;

    // Formula of these cells contains only numbers
    Concurrency::concurrent_vector<std::string> starting_cells;

    // DAG is directed acyclic graph. Edge 'a' -> 'b' exists if and only if formula of 'b' contains 'a'.
    // For each 'a' cell we store an array of nodes which are connected from 'a'.
    Concurrency::concurrent_unordered_map<std::string, Edges> DAG;

    Concurrency::concurrent_unordered_map<std::string, CellInfo*> cell_info;

    // Queue of jobs. Each job is to calculate something for the cell which is stored in queue.
    Concurrency::concurrent_queue<std::string> queue;

    // don't work set
    Concurrency::concurrent_unordered_map<std::string, bool> need_to_recalculate;
    Concurrency::concurrent_vector<std::string> top_sort_recalculations;

    std::atomic<int> calculated_cells_count = 0;

    void BuildDAG(bool parallel, const InputData& input_data);
    void SequentialBuildDAG(const InputData& input_data);
    void ParallelBuildDAG(const InputData& input_data);

    void RecalculateDAG(const std::string& cell, const Formula& formula);
    ValueType CalculateCellValue(const std::string& cell, const Formula& formula);

    void ParallelValuesCalculation();
    void InitialValuesCalculationThreadJob();


    void RecalculateCellsThreadJob();
    void TraverseDAGThreadJob();
    void BuildTopSortRecalculations(const std::string& cell);


public:

    // Time complexity is O(n) where n - number of vertices in input_data.
    void InitialCalculate(const InputData& input_data) override;

    // Time complexity is O(t) where t - total number of cells which are depended on 'cell'.
    void ChangeCell(const std::string& cell, const Formula& formula) override;

    OutputData GetCurrentValues() override;
};

#endif //SPREADSHEETENGINE_FAST_H