#include <algorithm>
#include <execution>
#include <cassert>
#include <functional>
#include <unordered_set>

#include "fast.h"
#include "../utils.h"


// -------------- Common methods--------------

inline ValueType FastSolution::CalculateCellValue(int cell, const Formula& formula) {
    ValueType value = 0;
    for (const auto& it : formula) {
        switch (it.type) {
            case Addend::CELL: {
                int addend_cell = it.value;
                auto addend = cell_info[addend_cell]->value.load();
                value = sum(value, addend.value);
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
    int threads_count = get_threads_count();

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

    auto add_edges = [&](const InputCellInfo& cell_info_io) {
        
        CellInfo* info = new CellInfo(cell_info_io.formula, cell_info_io.name);
        int cell = cell_info_io.id;

        cell_info[cell] = info;
        id_by_name[cell_info_io.name] = cell;

        bool just_value = true;
        for (const auto& formula_it : cell_info_io.formula) {
            if (formula_it.type == Addend::CELL) {
                int next = formula_it.value;
                DAG[next].push_back(OptionalCell(cell, false));
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

    while (calculated_cells_count.load(std::memory_order_acquire) < cells_count) {
        bool success = lock_free_queue.try_dequeue(cell);
        if (!success) {
            continue;
        }

        auto& c_info = cell_info[cell];

        auto cell_value = c_info->value.load();
        if (cell_value.is_calculated) {
            continue;
        }


        const auto& formula = c_info->formula;
        ValueType value = CalculateCellValue(cell, formula);

        bool expected = false;
        bool ok = c_info->value.compare_exchange_strong(cell_value, CellValue(true, value));
        if (!ok) {
            continue;
        }   
        
        calculated_cells_count.fetch_add(1);

        for (const auto& it : DAG[cell]) {
            int next = it.cell;
            const auto& next_info = cell_info.at(next);
                
            if (!next_info->value.load().is_calculated) {
                int unresolved_count = next_info->unresolved_cells_count.fetch_sub(1) - 1;
                if (unresolved_count == 0) {
                    lock_free_queue.enqueue(next);
                }
            }
        }
    }
}


void FastSolution::ParallelValuesCalculation() {
    queue.clear();
    lock_free_queue = moodycamel::BlockingConcurrentQueue<int>(2 * cell_info.size());
    for (const auto& it : starting_cells) {
        cell_info.at(it)->total_dependency_count = 1;
        lock_free_queue.enqueue(it);
    }
    
    runMultipleThreads([&]() { InitialValuesCalculationThreadJob(); });

#ifdef _DEBUG
    if (cell_info.size() != calculated_cells_count.load()) {
        std::cout << std::endl << "FAIL!!! calculated_cells_count is wrong" << std::endl;
        std::cout << "expected: " << cell_info.size() << std::endl;
        std::cout << "actual = " << calculated_cells_count.load() << std::endl;
        exit(1);
    }
#endif
}

void FastSolution::InitialCalculate(const InputData& input_data) {
    std::for_each(std::execution::par_unseq, std::begin(DAG), std::end(DAG), [](Edges& e) { e.clear(); });
    DAG.resize(input_data.size());
    cell_info.resize(input_data.size());
    starting_cells.clear();
    calculated_cells_count = 0;

    {
#ifdef _DEBUG
        Timer timer("        Parallel building DAG time: ");
#endif
        ParallelBuildDAG(input_data);
    }

    {
#ifdef _DEBUG
        Timer timer("        Parallel values calculation time: ");
#endif
        ParallelValuesCalculation();
    }
}

// -------------- Change formula of a cell --------------


void FastSolution::RecalculateDAG(int cell, const Formula& formula) {
    // Not really critical number of operations, we can do it in one thread.     
    for (const auto& formula_it : cell_info[cell]->formula) {
        if (formula_it.type == Addend::CELL) {
            for (auto& cell_it : DAG[formula_it.value]) {
                if (cell_it.cell == cell) {
                    cell_it.is_deleted = true;
                }
            }
        }
    }
    cell_info[cell]->formula = formula;

    for (const auto& formula_it : cell_info[cell]->formula) {
        if (formula_it.type == Addend::CELL) {
            DAG[formula_it.value].push_back(OptionalCell(cell, false));
        }
    }
}

void FastSolution::RecalculateCellsThreadJob() {
    int cells_count = cell_info.size();
    int cell;

    while (calculated_cells_count.load(std::memory_order_seq_cst) < cells_count) {
        bool success = lock_free_queue.try_dequeue(cell);
        if (!success) {
            continue;
        }

        auto& c_info = cell_info[cell];
        auto cell_value = c_info->value.load();

#ifdef _DEBUG
        if (c_info->unresolved_cells_count.load() != 0) {
            std::cout << "FAILED!!! unresolved_cells_count = " << c_info->unresolved_cells_count.load() << ", but should be 0" << std::endl;
            exit(1);
        }
#endif

        if (cell_value.is_calculated) {
            continue;
        }

        const auto& formula = c_info->formula;
        ValueType value = CalculateCellValue(cell, formula);
        
        bool ok = c_info->value.compare_exchange_strong(cell_value, CellValue(true, value));
        
        if (!ok) {
            continue;
        }

        calculated_cells_count++;
        
        for (const auto& it : DAG[cell]) {
            int next = it.cell;
            if (!it.is_deleted) {
                auto next_info = cell_info.at(next);
                int unresolved_count = next_info->unresolved_cells_count.fetch_sub(1) - 1;
                if (unresolved_count == 0) {
                    lock_free_queue.enqueue(next);
                }
            }
        }
    }
}

void FastSolution::FindRecalculationCellsThreadJob() {
    do {
        int cell;
        while (lock_free_queue.try_dequeue(cell)) {
            auto cell_value = cell_info.at(cell)->value.load();

            if (!cell_value.is_calculated) {
                continue;
            }

            bool ok = cell_info.at(cell)->value.compare_exchange_strong(cell_value, CellValue(false, 0));
            if (!ok) {
                continue;
            }

            count_to_recalculate++;
            need_to_recalculate.push_back(cell);

            for (const auto& it : DAG[cell]) {
                int next = it.cell;
                if (!it.is_deleted && cell_info.at(next)->value.load().is_calculated) {
                    lock_free_queue.enqueue(next);
                }
            }
        }
    } while (done_consumers.fetch_add(1, std::memory_order_acq_rel) + 1 == std::thread::hardware_concurrency());
}


void FastSolution::ChangeCell(const std::string& cell, const Formula& formula) {
    int cell_id = id_by_name[cell];
    RecalculateDAG(cell_id, formula);
     
    // Find cells which we need to recalculate
    {
#ifdef _DEBUG
        Timer timer("FindRecalculationCellsThreadJob time: ");
#endif
        count_to_recalculate = 0;
        lock_free_queue = moodycamel::BlockingConcurrentQueue<int>(2 * cell_info.size());
        lock_free_queue.enqueue(cell_id);
        done_consumers = 0;
        runMultipleThreads([&]() { FindRecalculationCellsThreadJob(); });
    }

    // Calculate number of cells in formula which are not calculated
    {
#ifdef _DEBUG
        Timer timer("count_unresolved_cells time: ");
#endif
        auto count_unresolved_cells = [&](int to_recalculate) {
            auto& info = cell_info.at(to_recalculate);
            int cnt = 0;
            for (const auto& formula_it : info->formula) {
                if (formula_it.type == Addend::CELL) {
                    int next = formula_it.value;
                    if (!cell_info.at(next)->value.load().is_calculated) {
                        cnt++;
                    }
                }
            }
            info->unresolved_cells_count.store(cnt);
        };
        auto lambda = [&](int it) { count_unresolved_cells(it); };
        std::for_each(std::execution::par_unseq, std::begin(need_to_recalculate), std::end(need_to_recalculate), lambda);
    }

#ifdef _DEBUG
    int cnt = 0;
    for (auto it : cell_info) {
        cnt += !it->value.load().is_calculated;
    }
    if (cnt != count_to_recalculate.load()) {
        std::cout << std::endl << "FAIL!!! [FindRecalculationCellsThreadJob] count_to_recalculate is wrong" << std::endl;
        std::cout << "actual = " << count_to_recalculate.load() << std::endl;
        std::cout << "expected = " << cnt << std::endl;
        exit(1);
    }
#endif
    
    // Recalculate cells
    {
#ifdef _DEBUG
        Timer timer("RecalculateCellsThreadJob time: ");
#endif
        lock_free_queue = moodycamel::BlockingConcurrentQueue<int>(2 * cell_info.size());
        lock_free_queue.enqueue(cell_id);
        calculated_cells_count = cell_info.size() - count_to_recalculate.load();
        runMultipleThreads([&]() { RecalculateCellsThreadJob(); });
    }

#ifdef _DEBUG
    for (auto it : cell_info) {
        if (!it->value.load().is_calculated) {
            std::cout << std::endl << "FAIL!!! [RecalculateCellsThreadJob] there is not calculated cell " << it->name << std::endl;
            exit(1);
        }
    }
#endif
}

// -------------- Return current state of cells --------------

OutputData FastSolution::GetCurrentValues() {
    OutputData result = OutputData();
    for (const auto& cell : cell_info) {
        result[cell->name] = cell->value.load().value;
    }
    return result;
}

// ----------------------------
