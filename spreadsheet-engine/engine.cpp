#include <iostream>
#include "reader.h"
#include "io-data.h"
#include "utils.h"
#include "solutions/one-thread-simple.h"
#include "solutions/fast.h"
#include "writer.h"
#include "solutions/solution.h"

// Compare two files and print detailed message if they are not equal.
inline bool check_correctness(const std::string& correct_file, const std::string& actual_file) {

    Timer timer("    File comparing time: ");
    std::string error_message;
    const std::string &f1 = correct_file;
    const std::string &f2 = actual_file;
    if (!files_are_equal(f1, f2, error_message)) {
        std::cout << "\nERROR: files " << f1 << " and " << f2 << " are not the same" << std::endl;
        std::cout << error_message << std::endl << std::endl;
        return false;
    }
    return true;
}

// Write solution result into file.
// Then check solution correctness if correct_solution_name is not empty.
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

bool test_solution(Solution& solution, const InputData& initial_data, const InputData& modifications_small_data,
                   const InputData& modifications_medium_data, const InputData& modifications_large_data,
                   const std::string& output_path, const std::string& solution_name,
                   const std::string& correct_solution_name) {

    std::cout << std::endl << solution_name << " solution:" << std::endl;


    // Calculate cell values according to initial_data.
    {
        Timer timer("    InitialCalculate method overall time: ");
        solution.InitialCalculate(initial_data);
    }
    if (!write_and_check(solution, output_path, solution_name, correct_solution_name, ".initial.txt")) {
        return false;
    }


    // Changing cells according to modifications_data (one by one).
    {
        Timer timer("    [small] ChangeCell method 1 call in average: ", modifications_small_data.size());
        for (const auto& it : modifications_small_data) {
            solution.ChangeCell(it.name, it.formula);
        }
    }
    if (!write_and_check(solution, output_path, solution_name, correct_solution_name, ".modifications_small.txt")) {
        return false;
    }

    {
        Timer timer("    [medium] ChangeCell method 1 call in average: ", modifications_medium_data.size());
        for (const auto& it : modifications_medium_data) {
            solution.ChangeCell(it.name, it.formula);
        }
    }
    if (!write_and_check(solution, output_path, solution_name, correct_solution_name, ".modifications_medium.txt")) {
        return false;
    }

    {
        Timer timer("    [large] ChangeCell method 1 call in average: ", modifications_large_data.size());
        for (const auto& it : modifications_large_data) {
            solution.ChangeCell(it.name, it.formula);
        }
    }
    if (!write_and_check(solution, output_path, solution_name, correct_solution_name, ".modifications_large.txt")) {
        return false;
    }
    
    return true;
}

bool test_solution(Solution& solution, const InputData& initial_data, const InputData& modifications_small_data,
                  const InputData& modifications_medium_data, const InputData& modifications_large_data,
                   const std::string& output_path, const std::string& solution_name) {

    return test_solution(solution, initial_data, modifications_small_data, modifications_medium_data, 
                         modifications_large_data, output_path, solution_name, "");
}

// Check if file can be opened.
inline bool validate_file(std::ifstream& s, const std::string& file_name) {
    if (!s.is_open()) {
        std::cout << "Unable to open file " << file_name << std::endl;
        s.close();
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    if (argc != 6) {
        std::cout << "Usage: engine.exe inputs/initial.txt inputs/modifications_small.txt inputs/modifications_medium.txt inputs/modifications_large.txt outputs/" << std::endl;
        return 1;
    }

    std::ifstream initial_file(argv[1]);
    if (!validate_file(initial_file, argv[1])) {
        return 1;
    }

    std::ifstream modifications_small_file(argv[2]);
    if (!validate_file(modifications_small_file, argv[2])) {
        return 1;
    }

    std::ifstream modifications_medium_file(argv[3]);
    if (!validate_file(modifications_medium_file, argv[3])) {
        return 1;
    }

    std::ifstream modifications_large_file(argv[4]);
    if (!validate_file(modifications_large_file, argv[4])) {
        return 1;
    }


    std::unordered_map<std::string, int> ids;

    InputData initial_data;
    {
        Timer timer("Reading initial file time: ");
        initial_data = Reader::Read(initial_file, ids);
        std::sort(initial_data.begin(), initial_data.end(), [](InputCellInfo& a, InputCellInfo& b) { return a.id < b.id; });
        initial_file.close();
    }

    InputData modifications_small_data;
    {
        Timer timer("Reading small modifications file time: ");
        modifications_small_data = Reader::Read(modifications_small_file, ids);
        modifications_small_file.close();
    }

    InputData modifications_medium_data;
    {
        Timer timer("Reading medium modifications file time: ");
        modifications_medium_data = Reader::Read(modifications_medium_file, ids);
        modifications_medium_file.close();
    }

    InputData modifications_large_data;
    {
        Timer timer("Reading large modifications file time: ");
        modifications_large_data = Reader::Read(modifications_large_file, ids);
        modifications_large_file.close();
    }

    std::string output_path = std::string(argv[5]);
    if (output_path.back() != '/' && output_path.back() != '\\') {
        output_path += '/';
    }

    // Run simple one thread solution.
    const std::string& correct_solution = "OneThreadSimple";
    
    {
        Solution* solution = new OneThreadSimpleSolution();
        bool success = test_solution(*solution, initial_data, modifications_small_data, modifications_medium_data,
            modifications_large_data, output_path, correct_solution);
        delete solution;
        if (!success) {
            return 1;
        }
    }

    {
        // Test fast solution, compare results to OneThreadSimple solution's output.
        Solution* solution = new FastSolution();
        bool success = test_solution(*solution, initial_data, modifications_small_data, modifications_medium_data, modifications_large_data,
            output_path, "FastSolution", correct_solution);
        delete solution;
        if (!success) {
            return 1;
        }
    }

    return 0;
}
