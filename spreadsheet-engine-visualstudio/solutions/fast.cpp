#include <algorithm>
#include <execution>
#include <cassert>
#include <functional>

#include "fast.h"
#include "../utils.h"

// -------------- Common methods--------------

inline ValueType FastSolution::CalculateCellValue(const std::string& cell, const Formula& formula) {
    ValueType value = 0;
    for (const auto& it : formula) {
        switch (it.type) {
            case Addend::CELL: {
                const std::string& addend_cell = it.value;
                auto& addend_info = cell_info.at(addend_cell);
                assert(addend_info->is_calculated.load());
                value = sum(value, addend_info->value.load());
                break;
            }

            case Addend::VALUE: {
                ValueType addend = to_value_type(it.value);
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

    auto add_edges = [&](const std::string& cell, const Formula& formula) {
        CellInfo* info = new CellInfo(formula);
        cell_info.insert(std::make_pair(cell, info));

        bool just_value = true;
        for (const auto& formula_it : formula) {
            if (formula_it.type == Addend::CELL) {
                std::string next = formula_it.value;
                DAG[next][cell] = false;
                just_value = false;
                info->unresolved_cells_count++;
            }
        }
        if (just_value) {
            starting_cells.push_back(cell);
        }
    };

    auto lambda = [&](std::pair<std::string, Formula> it) { add_edges(it.first, it.second); };

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

    while (calculated_cells_count.load() < cells_count) {
        std::string cell;
        bool success = queue.try_pop(cell);
        if (!success) {
            continue;
        }

        auto& c_info = cell_info.at(cell);

        if (c_info->is_calculated.load()) {
            continue;
        }

        const auto& formula = c_info->formula;
        ValueType value = 0;
        for (const auto& it : formula) {
            switch (it.type) {
            case Addend::CELL: {
                const std::string& addend_cell = it.value;
                auto& addend_info = cell_info.at(addend_cell);
                assert(addend_info->is_calculated.load());
                value = sum(value, addend_info->value.load());
                break;
            }

            case Addend::VALUE: {
                ValueType addend = to_value_type(it.value);
                value = sum(value, addend);
                break;
            }

            default:
                assert(false);
            }
        }
       
        if (!c_info->is_calculated.load()) {
            c_info->mutex.lock();
            if (!c_info->is_calculated.load()) {
                c_info->value.store(value);
                c_info->is_calculated.store(true);
                calculated_cells_count++;

                for (const auto& it : DAG[cell]) {
                    const std::string& next_str = it.first;
                    const auto& next = cell_info.at(next_str);
                    if (!next->is_calculated.load()) {
                        next->unresolved_cells_count--;
                        if (next->unresolved_cells_count.load() == 0) {
                            queue.push(it.first);
                        }
                    }
                }
            }
            c_info->mutex.unlock();
        }
    }
}


void FastSolution::ParallelValuesCalculation() {
   queue.clear();
    for (const auto& it : starting_cells) {
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
    {
        Timer timer("        Building DAG time: ");
        SequentialBuildDAG(input_data);
    }
#endif
    
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

void FastSolution::RecalculateDAG(const std::string& cell, const Formula& formula) {
    for (const auto& formula_it : cell_info[cell]->formula) {
        if (formula_it.type == Addend::CELL) {
            DAG[formula_it.value][cell] = true;
        }
    }
    cell_info[cell]->formula = formula;

    for (const auto& formula_it : cell_info[cell]->formula) {
        if (formula_it.type == Addend::CELL) {
            DAG[formula_it.value][cell] = false;
        }
    }
}

void FastSolution::RecalculateCellsThreadJob() {
    while (recalculation_count > 0 || !queue.empty()) {
        std::string cell;
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
                    const std::string& next = it.value;
                    const auto& next_info = cell_info.at(next);
                    if (!next_info->is_calculated.load()) {
                        can_calculate = false;
                    } else {
                        value = sum(value, next_info->value.load());
                    }

                    break;
                }

                case Addend::VALUE: {
                    ValueType addend = to_value_type(it.value);
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

        info->mutex.lock();
        info->value.store(value);
        info->is_calculated.store(true);
        info->mutex.unlock();

        recalculation_count--;

        for (auto it : DAG[cell]) {
            const std::string& next = it.first;
            if (!cell_info.at(it.first)->is_calculated.load()) {
                queue.push(next);
            }
        }
    }
}

void FastSolution::TraverseDAGThreadJob() {
    int retry_count = 10;
    while (retry_count > 0 || !queue.empty()) {
        std::string cell;
        bool success = queue.try_pop(cell);
        if (!success) {
            retry_count--;
            continue;
        }

        retry_count = 10;
        
        auto& info = cell_info.at(cell);

        if (need_to_recalculate.find(cell) != need_to_recalculate.end()) {
            continue;
        }

        need_to_recalculate.insert(std::make_pair(cell, true));
        info->is_calculated = false;

        for (auto it : DAG[cell]) {
            const std::string& next = it.first;
            queue.push(next);
        }
    }
}

void FastSolution::ChangeCell(const std::string& cell, const Formula& formula) {
    cell_info[cell]->formula = formula;

    RecalculateDAG(cell, formula);

    need_to_recalculate.clear();

    queue.clear();
    queue.push(cell);
    runMultipleThreads([&]() { TraverseDAGThreadJob(); });

    recalculation_count = need_to_recalculate.size();

    queue.clear();
    queue.push(cell);
    runMultipleThreads([&]() { RecalculateCellsThreadJob(); });
}

// -------------- Return current state of cells --------------

OutputData FastSolution::GetCurrentValues() {
    OutputData result = OutputData();
    for (const auto& it : cell_info) {
        result[it.first] = it.second->value;
    }
    return result;
}

// ----------------------------