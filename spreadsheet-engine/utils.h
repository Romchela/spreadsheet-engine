#ifndef SPREADSHEETENGINE_UTILS_H
#define SPREADSHEETENGINE_UTILS_H

#include <iostream>
#include <chrono>
#include <fstream>
#include <sstream>
#include <utility>
#include <thread>

// Class which calculate execution time. 
// When is's created it saves now().
// When class will be desctructed it prints time difference.
class Timer {
private:
    typedef std::chrono::high_resolution_clock clock_;
    std::chrono::time_point<clock_> start;
    std::string message;
    int count;

public:
    explicit Timer(std::string message) : start(clock_::now()), message(std::move(message)), count(1) {}
    explicit Timer(std::string message, int count) : start(clock_::now()), message(std::move(message)), count(count) {}
    ~Timer() {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(clock_::now() - start).count() / count;
        std::cout << message << elapsed  << " ms" << std::endl;
    }
};

inline std::string trim(std::string s) {
    s.erase(0, s.find_first_not_of(' '));
    s.erase(s.find_last_not_of(' ') + 1);
    return s;
}

inline unsigned int get_threads_count() {
  return std::thread::hardware_concurrency();
}

inline bool files_are_equal(const std::string& expected_path, const std::string& actual_path, std::string& error_message) {
    std::ifstream expected_file(expected_path);
    std::ifstream actual_file(actual_path);

    std::string e, a;
    bool equal = true;
    while (getline(expected_file, e)) {
        if (!getline(actual_file, a)) {
            error_message = "Expected file size is greater than actual file size";
            equal = false;
            break;
        }
        if (e != a) {
            error_message = "Expected:\n    " + e + "\nActual:\n    " + a;
            equal = false;
            break;
        }
    }

    if (equal && getline(actual_file, a)) {
        error_message = "Actual file size is greater than expected file size";
        equal = false;
    }

    expected_file.close();
    actual_file.close();
    return equal;
}

#endif //SPREADSHEETENGINE_UTILS_H
