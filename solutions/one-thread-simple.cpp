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
                    assert(next_cell.is_calculated); // TODO: cycle check
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

void OneThreadSimpleSolution::Recalculate(const std::string& cell) {
    ValueType value = 0;
    for (const auto& it : cells[cell].formula) {
        switch (it.type) {
            case Addend::CELL: {
                const std::string& next = it.value;
                value = sum(value, cells[next].value);
                break;
            }

            case Addend::VALUE:
                value += to_value_type(it.value);
                break;

            default:
                assert(false);
        }
    }
    cells[cell].value = value;

    for (const auto& it : cells[cell].dependencies) {
        Recalculate(it);
    }
}

void OneThreadSimpleSolution::RecalculateDependencies(const std::string& cell) {
    for (const auto& formula_it : cells[cell].formula) {
        if (formula_it.type == Addend::CELL) {
            cells[formula_it.value].dependencies.push_back(cell);
        }
    }
}

void OneThreadSimpleSolution::InitialCalculate(const InputData& initial_data) {
    for (const auto& it : initial_data) {
        cells[it.first].formula = it.second;
        RecalculateDependencies(it.first);
    }

    for (const auto& it : initial_data) {
        Calculate(it.first);
    }
}

void OneThreadSimpleSolution::ChangeCell(const std::string& cell, const Formula& formula) {
    auto& c = cells[cell];
    c.formula = formula;

    RecalculateDependencies(cell);
    Recalculate(cell);
}

OutputData OneThreadSimpleSolution::GetCurrentValues() {
    OutputData result = OutputData();
    for (const auto& cell : cells) {
        result[cell.first] = cell.second.value;
    }
    return result;
}


