#ifndef SPREADSHEETENGINE_FAST_H
#define SPREADSHEETENGINE_FAST_H

#include <mutex>
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>
#include <concurrent_queue.h>

#include "solution.h"
#include "../blockingconcurrentqueue.h"

// This solution based on bfs (breadth-first search) which is easy to parallelize.
// InitialCalculate() and ChangeCell() methods have the same time and space complexity as OneThreadSimple solution has,
// but FastSolution is much faster because we do work in multiple threads.
//
// We use collections from Concurrency namespace because they are thread-safe and efficient for parallel access/modifications.
// More details: https://docs.microsoft.com/ru-ru/cpp/parallel/concrt/parallel-containers-and-objects
class FastSolution : public Solution {
private:

    using IsDeleted = bool;
    using Edges = Concurrency::concurrent_vector<std::pair<int, IsDeleted>>;
    struct CellInfo {
        CellInfo() = default;
        CellInfo(const Formula& formula, const std::string& name) : 
            formula(formula), is_calculated(false), unresolved_cells_count(0), name(name), total_dependency_count(0) {}

        std::mutex mutex;
        std::atomic<ValueType> value;
        std::atomic<bool> is_calculated;
        Formula formula;
        std::atomic<int> unresolved_cells_count;
        std::string name;
        std::atomic<int> total_dependency_count;
    };
    
    InputData state;
    std::atomic<int> recalculation_count = 0;

    Concurrency::concurrent_unordered_map<std::string, int> id_by_name;

    // Formula of these cells contains only numbers
    Concurrency::concurrent_vector<int> starting_cells;

    // DAG is directed acyclic graph. Edge 'a' -> 'b' exists if and only if formula of 'b' contains 'a'.
    // For each 'a' cell we store an array of nodes which are connected from 'a'.
    std::vector<Edges> DAG;

    std::vector<CellInfo*> cell_info;

    // Queue of jobs. Each job is to calculate something for the cell which is stored in queue.
    Concurrency::concurrent_queue<int> queue;

    std::atomic<int> count_to_recalculate;

    // concurrent_unordered_set doesn't work in C++17 (it uses deprecated method).
    // Use concurrent_unordered_map instead (value is useless param here).
    Concurrency::concurrent_unordered_map<int, bool> need_to_recalculate;

    std::atomic<int> calculated_cells_count = 0;

    void BuildDAG(bool parallel, const InputData& input_data);
    // For testing purpose
    void SequentialBuildDAG(const InputData& input_data);
    void ParallelBuildDAG(const InputData& input_data);

    void RecalculateDAG(int cell, const Formula& formula);
    ValueType CalculateCellValue(int cell, const Formula& formula);

    void ParallelValuesCalculation();
    void InitialValuesCalculationThreadJob();

    void RecalculateCellsThreadJob();
    void TraverseDAGThreadJob();

public:

    // Time complexity is O(n) where n - number of vertices in input_data.
    void InitialCalculate(const InputData& input_data) override;

    // Time complexity is O(t) where t - total number of cells which are depended on 'cell'.
    void ChangeCell(const std::string& cell, const Formula& formula) override;

    OutputData GetCurrentValues() override;

    ~FastSolution() {
        for (const auto& it : cell_info) {
            delete it;
        }
        cell_info.clear();
    }
};

#endif //SPREADSHEETENGINE_FAST_H
