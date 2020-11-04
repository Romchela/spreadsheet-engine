#include <algorithm>
#include <execution>
#include <cassert>
#include <functional>

#include "fast.h"
#include "../utils.h"

// -------------- Common methods--------------

inline ValueType FastSolution::CalculateCellValue(int cell, const Formula& formula) {
    ValueType value = 0;
    for (const auto& it : formula) {
        switch (it.type) {
            case Addend::CELL: {
                int addend_cell = it.value;
                auto& addend_info = cell_info[addend_cell];
                assert(addend_info->is_calculated.load());
                value = sum(value, addend_info->value.load());
                break;
            }

            case Addend::VALUE: {
                ValueType addend = it.value;
                value = sum(value, addend);
                break;
            }

            default:
                assert(false);
            }
    }
    return value;
}

inline void runMultipleThreads(const std::function<void()>& function) {
    int threads_count = std::thread::hardware_concurrency();
#ifdef _DEBUG
    std::cout << "[DEBUG] Threads count = " << threads_count << std::endl;
#endif


    std::vector<std::thread> threads;
    for (int i = 0; i < threads_count; i++) {
        threads.push_back(std::thread(function));
    }

    for (int i = 0; i < threads_count; i++) {
        threads[i].join();
    }
}

// -------------- DAG building --------------

void FastSolution::BuildDAG(bool parallel, const InputData& input_data) {
    DAG.clear();
    cell_info.clear();
    id_by_name.clear();
    DAG.resize(input_data.size());
    cell_info.resize(input_data.size());
    starting_cells.clear();

    auto add_edges = [&](const InputCellInfo& cell_info_io) {
        CellInfo* info = new CellInfo(cell_info_io.formula, cell_info_io.name);
        int cell = cell_info_io.id;
        cell_info[cell] = info;
        id_by_name[cell_info_io.name] = cell;

        bool just_value = true;
        for (const auto& formula_it : cell_info_io.formula) {
            if (formula_it.type == Addend::CELL) {
                int next = formula_it.value;
                DAG[next].push_back(std::make_pair(cell, false));
                just_value = false;
                info->unresolved_cells_count++;
            }
        }
        if (just_value) {
            starting_cells.push_back(cell);
        }
    };

    auto lambda = [&](InputCellInfo it) { add_edges(it); };

    if (parallel) {
        std::for_each(std::execution::par_unseq, std::begin(input_data), std::end(input_data), lambda);
    } else {
        std::for_each(std::execution::seq, std::begin(input_data), std::end(input_data), lambda);
    }
}

void FastSolution::SequentialBuildDAG(const InputData& input_data) {
    BuildDAG(false, input_data);
}

void FastSolution::ParallelBuildDAG(const InputData& input_data) {
    BuildDAG(true, input_data);
}

// -------------- Initial values calculation --------------

void FastSolution::InitialValuesCalculationThreadJob() {
    int cells_count = cell_info.size();
    int cell;

    while (calculated_cells_count.load() < cells_count) {
        bool success = queue.try_pop(cell);
        if (!success) {
            continue;
        }

        auto& c_info = cell_info[cell];

        if (c_info->is_calculated.load()) {
            continue;
        }

        const auto& formula = c_info->formula;
        ValueType value = 0;
        for (const auto& it : formula) {
            switch (it.type) {
            case Addend::CELL: {
                int addend_cell = it.value;
                auto& addend_info = cell_info.at(addend_cell);
                assert(addend_info->is_calculated.load());
                value = sum(value, addend_info->value.load());
                break;
            }

            case Addend::VALUE: {
                ValueType addend = it.value;
                value = sum(value, addend);
                break;
            }

            default:
                assert(false);
            }
        }
       

        bool expected = false;
        bool ok = c_info->is_calculated.compare_exchange_strong(expected, true);
        if (!ok) {
            continue;
        }   
        
        c_info->value.store(value);
        calculated_cells_count++;

        for (const auto& it : DAG[cell]) {
            int next_str = it.first;
            const auto& next = cell_info.at(next_str);
                
            if (!next->is_calculated.load()) {
                next->unresolved_cells_count--;
                if (next->unresolved_cells_count.load() == 0) {
                    queue.push(it.first);
                }
            }
        }
    }
}


void FastSolution::ParallelValuesCalculation() {
    queue.clear();
    for (const auto& it : starting_cells) {
        cell_info.at(it)->total_dependency_count = 1;
        queue.push(it);
    }
    
    runMultipleThreads([&]() { InitialValuesCalculationThreadJob(); });

#ifdef _DEBUG
    std::cout << "[DEBUG] cells count = " << cell_info.size() << std::endl;
    std::cout << "[DEBUG] calculated_cells_count = " << calculated_cells_count.load() << std::endl;
#endif
}

void FastSolution::InitialCalculate(const InputData& input_data) {
#ifdef _DEBUG
    /*{
        Timer timer("        Building DAG time: ");
        SequentialBuildDAG(input_data);
    }*/
#endif

    state = input_data;

    {
        Timer timer("        Parallel building DAG time: ");
        ParallelBuildDAG(input_data);
    }

    {
        Timer timer("        Parallel values calculation time: ");
        ParallelValuesCalculation();
    }
}

// -------------- Change formula of a cell --------------

// Not really critical number of operations, we can do it in one thread.
void FastSolution::RecalculateDAG(int cell, const Formula& formula) {
     
    for (const auto& formula_it : cell_info[cell]->formula) {
        if (formula_it.type == Addend::CELL) {
            for (auto& cell_it : DAG[formula_it.value]) {
                if (cell_it.first == cell) {
                    cell_it.second = true;
                }
            }
        }
    }
    cell_info[cell]->formula = formula;

    for (const auto& formula_it : cell_info[cell]->formula) {
        if (formula_it.type == Addend::CELL) {
            DAG[formula_it.value].push_back(std::make_pair(cell, false));
        }
    }
}

void FastSolution::RecalculateCellsThreadJob() {
    int cell;
    while (!queue.empty()) {
        bool success = queue.try_pop(cell);
        if (!success) {
            continue;
        }

        auto& info = cell_info.at(cell);

        
        if (info->is_calculated.load()) {
            continue;
        }

        bool can_calculate = true;
        ValueType value = 0;

        for (const auto& it : info->formula) {
            switch (it.type) {
                case Addend::CELL: {
                    int next = it.value;
                    const auto& next_info = cell_info.at(next);
                    if (!next_info->is_calculated.load()) {
                        can_calculate = false;
                    } else {
                        value = sum(value, next_info->value.load());
                    }

                    break;
                }

                case Addend::VALUE: {
                    ValueType addend = it.value;
                    value = sum(value, addend);
                    break;
                }

                default:
                    assert(false);
            }

            if (!can_calculate) {
                break;
            }
        }

        if (!can_calculate) {
            queue.push(cell);
            continue;
        }

        bool expected = false;
        bool ok = info->is_calculated.compare_exchange_strong(expected, true);
        if (!ok) {
            continue;
        }
        info->value.store(value);
       
        recalculation_count--;

        for (const auto& it : DAG[cell]) {
            int next = it.first;
            if (!it.second && !cell_info.at(next)->is_calculated.load()) {
                queue.push(next);
            }
        }
    }
}

void FastSolution::TraverseDAGThreadJob() {
    int cell;
    while (!queue.empty()) {
        bool success = queue.try_pop(cell);
        if (!success) {
            continue;
        }

        auto& info = cell_info.at(cell);

        bool expected = true;
        bool ok = cell_info.at(cell)->is_calculated.compare_exchange_strong(expected, false);
        if (!ok) {
            continue;
        }

        count_to_recalculate++;

        for (const auto& it : DAG[cell]) {
            int next = it.first;
            if (!it.second && cell_info.at(next)->is_calculated.load()) {
                queue.push(next);
            }
        }
    }
}

void FastSolution::ChangeCell(const std::string& cell, const Formula& formula) {
    int cell_id = id_by_name[cell];
    state[cell_id].formula = formula;
    
    RecalculateDAG(cell_id, formula);

    queue.clear();
    queue.push(cell_id);
    count_to_recalculate = 0;

    {
        //Timer timer("TraverseDAGThreadJob time: ");
        runMultipleThreads([&]() { TraverseDAGThreadJob(); });
    }
    
    queue.clear();
    queue.push(cell_id);
    {
        //Timer timer("RecalculateCellsThreadJob time: ");
        runMultipleThreads([&]() { RecalculateCellsThreadJob(); });
    }
}

// -------------- Return current state of cells --------------

OutputData FastSolution::GetCurrentValues() {
    OutputData result = OutputData();
    for (const auto& cell : cell_info) {
        result[cell->name] = cell->value;
    }
    return result;
}

// ----------------------------