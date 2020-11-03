#ifndef SPREADSHEETENGINE_READER_H
#define SPREADSHEETENGINE_READER_H

#include <iostream>
#include <fstream>
#include "io-data.h"

class ParserException: public std::exception {
private:
    std::string message;
public:
    const char* what() const noexcept override {
        return message.c_str();
    }

    explicit ParserException(const std::string& m) {
        message = m;
    }
};

class Reader {
public:
    static InputData Read(std::ifstream& input_file);
};

#endif //SPREADSHEETENGINE_READER_H
