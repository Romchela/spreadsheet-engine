#include <algorithm>
#include <execution>
#include <cassert>

#include "fast.h"
#include "../utils.h"


void FastSolution::BuildDAG(const InputData& input_data) {
    DAG.clear();

    for (const auto& input_data_it : input_data) {
        const std::string& cell = input_data_it.first;
        const Formula& formula = input_data_it.second;

        CellInfo* info = new CellInfo(formula);
        cell_info.insert(std::make_pair(cell, info));

        bool just_value = true;
        for (const auto& formula_it : formula) {
            if (formula_it.type == Addend::CELL) {
                const std::string& next_cell = formula_it.value;
                DAG[next_cell].push_back(cell);
                just_value = false;
            }
        }
        if (just_value) {
            starting_cells.push_back(cell);
        }
    }
}

void FastSolution::ParallelBuildDAG(const InputData& input_data) {
    DAG.clear();
    auto add_edges = [&](const std::pair<std::string, Formula>& input_data_it) {
        const std::string& cell = input_data_it.first;
        const Formula& formula = input_data_it.second;

        CellInfo* info = new CellInfo(formula);
        cell_info.insert(std::make_pair(cell, info));

        bool just_value = true;
        for (const auto& formula_it : formula) {
            if (formula_it.type == Addend::CELL) {
                const std::string& next_cell = formula_it.value;
                auto find_it = DAG.find(next_cell);
                auto& d = DAG[next_cell];
                d.push_back(cell);
                just_value = false;
            }
        }
        if (just_value) {
            starting_cells.push_back(cell);
        }
    };

    std::for_each(std::execution::par_unseq, std::begin(input_data), std::end(input_data), add_edges);
}

void FastSolution::ThreadJob() {
    while (calculated_cells_count.load() < cell_info.size()) {
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
        bool calculated = true;
        ValueType value = 0;

        for (const auto& it : formula) {
            switch (it.type) {
            case Addend::CELL: {
                const std::string& addend_cell = it.value;
                auto& addend_info = cell_info.at(addend_cell);
                if (addend_info->is_calculated.load()) {
                    value = sum(value, addend_info->value.load());
                }
                else {
                    calculated = false;
                    break;
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

            if (!calculated) {
                break;
            }
        }

        if (calculated && !c_info->is_calculated.load()) {
            c_info->mutex.lock();
            if (!c_info->is_calculated.load()) {
                c_info->value.store(value);
                c_info->is_calculated.store(true);
                calculated_cells_count++;

                for (const auto& it : DAG[cell]) {
                    if (!cell_info.at(it)->is_calculated.load()) {
                        queue.push(it);
                    }
                }
            }
            c_info->mutex.unlock();
        }
    }
}

void FastSolution::ParallelValuesCalculation() {
    for (const auto& it : starting_cells) {
        queue.push(it);
    }

    int threads_count = std::thread::hardware_concurrency();

#ifdef _DEBUG
    std::cout << "[DEBUG] Threads count = " << threads_count << std::endl;
#endif

    std::vector<std::thread> threads;
    for (int i = 0; i < threads_count; i++) {
        threads.push_back(std::thread([&]() { ThreadJob(); }));
    }
    
    for (int i = 0; i < threads_count; i++) {
        threads[i].join();
    }

#ifdef _DEBUG
    std::cout << "[DEBUG] cells count = " << cell_info.size() << std::endl;
    std::cout << "[DEBUG] calculated_cells_count = " << calculated_cells_count.load() << std::endl;
#endif
}

void FastSolution::InitialCalculate(const InputData& input_data) {
#ifdef _DEBUG
    {
        Timer timer("        Building DAG time: ");
        BuildDAG(input_data);
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

void FastSolution::ChangeCell(const std::string& cell, const Formula& formula) {

}

OutputData FastSolution::GetCurrentValues() {
    OutputData result = OutputData();
    for (const auto& it : cell_info) {
        result[it.first] = it.second->value;
    }
    return result;
}