#include <cassert>
#include "one-thread-simple.h"

void OneThreadSimpleSolution::Calculate(const std::string& cell) {
    ValueType value = 0;
    for (const auto& it : cells[cell].formula) {
        switch (it.type) {
            case Addend::CELL: {
                const std::string& next = it.value;
                auto next_it = cells.find(next);
                assert(next_it != cells.end());
                auto& next_cell = next_it->second;

                if (!next_cell.is_calculated) {
                    Calculate(next);
                    assert(next_cell.is_calculated); // TODO: assert fails if cycle exists; for now we don't allow such cases
                }

                value = sum(value, next_cell.value);
                break;
            }

            case Addend::VALUE:
                value += to_value_type(it.value);
                break;

            default:
                assert(false);
        }
    }

    auto& c = cells[cell];
    c.is_calculated = true;
    c.value = value;
}

void OneThreadSimpleSolution::BuildTopSortRecalculations(const std::string& cell) {
    if (need_to_recalculate.find(cell) != need_to_recalculate.end()) {
        return;
    }

    need_to_recalculate.insert(cell);
    for (const auto& it : cells[cell].dependencies) {
        BuildTopSortRecalculations(it);
    }
    top_sort_recalculations.push_back(cell);
    cells[cell].is_calculated = false;
}

void OneThreadSimpleSolution::RecalculateDependencies(const std::string& cell, const Formula& formula) {
    for (const auto& formula_it : cells[cell].formula) {
        if (formula_it.type == Addend::CELL) {
            cells[formula_it.value].dependencies.erase(cell);
        }
    }

    cells[cell].formula = formula;
    CalculateDependencies(cell);
}

void OneThreadSimpleSolution::CalculateDependencies(const std::string& cell) {
    for (const auto& formula_it : cells[cell].formula) {
        if (formula_it.type == Addend::CELL) {
            cells[formula_it.value].dependencies.insert(cell);
        }
    }
}

void OneThreadSimpleSolution::InitialCalculate(const InputData& initial_data) {
    for (const auto& it : initial_data) {
        cells[it.first].formula = it.second;
        CalculateDependencies(it.first);
    }

    for (const auto& it : initial_data) {
        Calculate(it.first);
    }
}

void OneThreadSimpleSolution::ChangeCell(const std::string& cell, const Formula& formula) {
    RecalculateDependencies(cell, formula);
    need_to_recalculate.clear();
    top_sort_recalculations.clear();
    BuildTopSortRecalculations(cell);
    for (const auto& it : top_sort_recalculations) {
        Calculate(it);
    }
}

OutputData OneThreadSimpleSolution::GetCurrentValues() {
    OutputData result = OutputData();
    for (const auto& cell : cells) {
        result[cell.first] = cell.second.value;
    }
    return result;
}


