#include <cassert>
#include "one-thread-simple.h"

void OneThreadSimpleSolution::Calculate(int cell) {
    ValueType value = 0;
    for (const auto& it : cells[cell].formula) {
        switch (it.type) {
            case Addend::CELL: {
                int next = it.value;
                auto& next_cell = cells[next];

                if (!next_cell.is_calculated) {
                    Calculate(next);
                    assert(next_cell.is_calculated); // TODO: assert fails if cycle exists; for now we don't allow such cases
                }

                value = sum(value, next_cell.value);
                break;
            }

            case Addend::VALUE:
                value += it.value;
                break;

            default:
                assert(false);
        }
    }

    auto& c = cells[cell];
    c.is_calculated = true;
    c.value = value;
}

void OneThreadSimpleSolution::BuildTopSortRecalculations(int cell) {
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

void OneThreadSimpleSolution::RecalculateDependencies(int cell, const Formula& formula) {
    for (const auto& formula_it : cells[cell].formula) {
        if (formula_it.type == Addend::CELL) {
            cells[formula_it.value].dependencies.erase(cell);
        }
    }

    cells[cell].formula = formula;
    CalculateDependencies(cell);
}

void OneThreadSimpleSolution::CalculateDependencies(int cell) {
    for (const auto& formula_it : cells[cell].formula) {
        if (formula_it.type == Addend::CELL) {
            cells[formula_it.value].dependencies.insert(cell);
        }
    }
}

void OneThreadSimpleSolution::InitialCalculate(const InputData& initial_data) {
    cells.resize(initial_data.size());

    for (const auto& it : initial_data) {
        cells[it.id].formula = it.formula;
        cells[it.id].name = it.name;
        CalculateDependencies(it.id);
        id_by_name[it.name] = it.id;
    }

    for (const auto& it : initial_data) {
        Calculate(it.id);
    }
}

void OneThreadSimpleSolution::ChangeCell(const std::string& cell, const Formula& formula) {
    int cell_id = id_by_name[cell];
    RecalculateDependencies(cell_id, formula);
    need_to_recalculate.clear();
    top_sort_recalculations.clear();
    BuildTopSortRecalculations(cell_id);
    for (const auto& it : top_sort_recalculations) {
        Calculate(it);
    }
}

OutputData OneThreadSimpleSolution::GetCurrentValues() {
    OutputData result = OutputData();
    int id = 0;
    for (const auto& cell : cells) {
        result[cell.name] = cell.value;
        id++;
    }
    return result;
}


