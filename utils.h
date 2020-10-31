#ifndef SPREADSHEETENGINE_UTILS_H
#define SPREADSHEETENGINE_UTILS_H

#include <iostream>
#include <chrono>
#include <sstream>
#include <utility>

class Timer {
private:
    typedef std::chrono::high_resolution_clock clock_;
    std::chrono::time_point<clock_> start;
    std::string message;

public:
    explicit Timer(std::string  message) : start(clock_::now()), message(std::move(message)) {}
    ~Timer() {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(clock_::now() - start).count();
        std::cout << message << elapsed << " ms" << std::endl;
    }
};

// TODO: use std::to_string instead (mingw bug https://sourceforge.net/p/mingw-w64/mailman/message/36948917/)
template<typename T> std::string to_str(const T &t) {
    std::stringstream ss;
    ss << t;
    return ss.str();
}

inline std::string trim(std::string s) {
    s.erase(0, s.find_first_not_of(' '));
    s.erase(s.find_last_not_of(' ') + 1);
    return s;
}

#endif //SPREADSHEETENGINE_UTILS_H