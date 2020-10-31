#include <iostream>
#include "reader.h"
#include "io-data.h"
#include "utils.h"
#include "solutions/one-thread-simple.h"
#include "writer.h"

void test_solution(Solution& solution, InputData& input_data, const std::string& solution_name) {
    std::cout << solution_name << " solution:" << std::endl;
    {
        Timer timer("    InitialCalculate time: ");
        solution.InitialCalculate(input_data);
    }

    Writer writer;
    writer.write(solution.GetCurrentValues(), solution_name + "_initial.txt");
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: engine input.txt" << std::endl;
        return 1;
    }

    std::ifstream input_file(argv[1]);
    if (!input_file.is_open()) {
        std::cout << "Unable to open file " << argv[1] << std::endl;
        input_file.close();
        return 1;
    }

    InputData input_data;
    {
        Timer timer("Reading input file time: ");
        input_data = Reader::Read(input_file);
    }

    test_solution(*new OneThreadSimpleSolution(), input_data, "OneThreadSimple");


    input_file.close();

    return 0;
}
