#include <iostream>
#include "reader.h"
#include "utils.h"

using Tokens = std::vector<std::string>;

Tokens split(const std::string& s, char delimiter) {
    Tokens tokens = Tokens();
    std::size_t start = 0;
    std::size_t end = s.find(delimiter);
    while (end != std::string::npos) {
        tokens.push_back(trim(s.substr(start, end - start)));
        start = end + 1;
        end = s.find(delimiter, start);
    }

    tokens.push_back(trim(s.substr(start, end)));
    return tokens;
}

// Correct cell format is ['A'-'Z'] ['0'-'9']*
bool validate_cell(const std::string& s) {
    std::string t = trim(s);
    if ('A' <= t[0] && t[0] <= 'Z') {
        try {
            to_value_type(t.substr(1, t.length() - 1));
            return true;
        } catch (std::exception& ignored) {
        }
    }
    return false;
}

InputData Reader::Read(std::ifstream& input_file) {
    std::string line;
    InputData input_data = InputData();
    int line_num = 1;
    while (std::getline(input_file, line)) {
        // Parse cell
        Tokens tokens = split(line, '=');
        if (tokens.size() != 2) {
            throw ParserException("Unable to find \'=\' in line " + std::to_string(line_num));
        }
        const std::string& cell = tokens[0];
        if (!validate_cell(cell)) {
            throw ParserException("Wrong cell format " + cell);
        }
        if (input_data.find(cell) != input_data.end()) {
            throw ParserException("Cell  " + tokens[0] + " initialized twice");
        }

        // Parse formula
        Tokens addend_tokens = split(tokens[1], '+');
        for (const auto& it : addend_tokens) {
            if (validate_cell(it)) {
                input_data[cell].push_back(AddendFactory::CellAddend(it));
            } else {
                try {
                    input_data[cell].push_back(AddendFactory::ValueAddend(it));
                }
                catch (std::exception& e) {
                    throw ParserException("Addend " + it + " is not cell or value");
                }
            }
        }

        line_num++;
    }
    return input_data;
}

