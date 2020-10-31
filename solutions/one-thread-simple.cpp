#include "one-thread-simple.h"
#include <ctime>

void OneThreadSimpleSolution::InitialCalculate(const InputData& inputData) {

}

void OneThreadSimpleSolution::ChangeCell(const std::string& cell, Formula formula) {

}

OutputData OneThreadSimpleSolution::GetCurrentValues() {
    OutputData result = OutputData();
    result["I23589"] = 23;
    return result;
}

