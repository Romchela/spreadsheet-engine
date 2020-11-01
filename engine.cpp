#include <iostream>
#include "reader.h"
#include "io-data.h"
#include "utils.h"
#include "solutions/one-thread-simple.h"
#include "solutions/fast.h"
#include "writer.h"
#include "solutions/solution.h"

inline bool check_correctness(const std::string& correct_solution_name, const std::string& solution_name) {
    Timer timer("    File comparing time: ");
    std::string error_message;
    const std::string &f1 = correct_solution_name;
    const std::string &f2 = solution_name;
    if (!files_are_equal(f1, f2, error_message)) {
        std::cout << "\nERROR: files " << f1 << " and " << f2 << " are not the same" << std::endl;
        std::cout << error_message << std::endl << std::endl;
        return false;
    }
    return true;
}

bool write_and_check(Solution& solution, const std::string& output_path, const std::string& solution_name,
                     const std::string& correct_solution_name, const std::string& extension) {

    bool compare_to_correct_solution = correct_solution_name.length() > 0;
    const std::string& cur_file_path = output_path + solution_name + extension;
    {
        Timer timer("    Writing in file time: ");
        Writer::write(solution.GetCurrentValues(), cur_file_path);
    }

    if (!compare_to_correct_solution) {
        return true;
    }

    const std::string& correct_file_path = output_path + correct_solution_name + extension;
    return check_correctness(correct_file_path, cur_file_path);
}

bool test_solution(Solution& solution, const InputData& initial_data, const InputData& modifications_data,
                  const std::string& output_path, const std::string& solution_name,
                  const std::string& correct_solution_name) {

    std::cout << solution_name << " solution:" << std::endl;


    // Calculate cell values according to initial_data
    {
        Timer timer("    InitialCalculate method time: ");
        solution.InitialCalculate(initial_data);
    }
    if (!write_and_check(solution, output_path, solution_name, correct_solution_name, ".initial.txt")) {
        return false;
    }


    // Changing cells according to modifications_data
    {
        Timer timer("    ChangeCell method overall time: ");
        for (const auto& it : modifications_data) {
            solution.ChangeCell(it.first, it.second);
        }
    }
    if (!write_and_check(solution, output_path, solution_name, correct_solution_name, ".modifications.txt")) {
        return false;
    }

    std::cout << std::endl;
    return true;
}

bool test_solution(Solution& solution, const InputData& initial_data, const InputData& modifications_data,
                  const std::string& output_path, const std::string& solution_name) {

    return test_solution(solution, initial_data, modifications_data, output_path, solution_name, "");
}

// Check if file is ok
inline bool validate_file(std::ifstream& s, const std::string& file_name) {
    if (!s.is_open()) {
        std::cout << "Unable to open file " << file_name << std::endl;
        s.close();
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cout << "Usage: engine inputs/initial.txt inputs/modifications.txt outputs/" << std::endl;
        return 1;
    }

    std::ifstream initial_file(argv[1]);
    if (!validate_file(initial_file, argv[1])) {
        return 1;
    }

    std::ifstream modifications_file(argv[2]);
    if (!validate_file(modifications_file, argv[2])) {
        return 1;
    }

    InputData initial_data;
    {
        Timer timer("Reading initial file time: ");
        initial_data = Reader::Read(initial_file);
        initial_file.close();
    }

    InputData modifications_data;
    {
        Timer timer("Reading modifications file time: ");
        modifications_data = Reader::Read(modifications_file);
        modifications_file.close();
    }

    std::string output_path = std::string(argv[3]);
    if (output_path.back() != '/' && output_path.back() != '\\') {
        output_path += '/';
    }

    // Run simple one thread solution
    const std::string& correct_solution = "OneThreadSimple";
    bool success = test_solution(
            *new OneThreadSimpleSolution(), initial_data, modifications_data,
            output_path, correct_solution);
    if (!success) {
        return 1;
    }

    // Test fast solution, compare results to correct_solution's outputs
    success = test_solution(
            *new FastSolution(), initial_data, modifications_data,
            output_path,"FastSolution", correct_solution);
    if (!success) {
        return 1;
    }

    return 0;
}
