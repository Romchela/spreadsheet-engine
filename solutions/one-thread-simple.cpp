#include <cassert>
#include "one-thread-simple.h"

void OneThreadSimpleSolution::calculate(const std::string& cell) {
    ValueType value = 0;
    for (const auto& it : cells[cell].formula) {
        switch (it.type) {
            case Addend::CELL: {
                const std::string& next = it.value;
                auto next_it = cells.find(next);
                assert(next_it != cells.end());
                auto& next_cell = next_it->second;

                if (!next_cell.is_calculated) {
                    calculate(next);
                    assert(next_cell.is_calculated); // TODO: cycle check
                }

                value += next_cell.value;
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

void OneThreadSimpleSolution::recalculate_dependencies(const std::string& cell) {
    for (const auto& formula_it : cells[cell].formula) {
        if (formula_it.type == Addend::CELL) {
            cells[formula_it.value].dependencies.push_back(cell);
        }
    }
}

void OneThreadSimpleSolution::InitialCalculate(const InputData& initial_data) {
    for (const auto& it : initial_data) {
        cells[it.first].formula = it.second;
        recalculate_dependencies(it.first);
    }

    for (const auto& it : initial_data) {
        calculate(it.first);
    }
}

void OneThreadSimpleSolution::ChangeCell(const std::string& cell, const Formula& formula) {
    auto& c = cells[cell];
    c.formula = formula;

    calculate(cell);
    recalculate_dependencies(cell);

    for (const auto& dependency : c.dependencies) {
        calculate(dependency);
    }
}

OutputData OneThreadSimpleSolution::GetCurrentValues() {
    OutputData result = OutputData();
    for (const auto& cell : cells) {
        result[cell.first] = cell.second.value;
    }
    return result;
}


