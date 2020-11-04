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


InputData Reader::Read(std::ifstream& input_file, std::unordered_map<std::string, int>& ids) {
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
        /*if (input_data.find(cell) != input_data.end()) {
            throw ParserException("Cell  " + tokens[0] + " initialized twice");
        }*/

        int id;
        if (ids.find(cell) == ids.end()) {
            id = ids.size();
            ids[cell] = ids.size();
        } else {
            id = ids[cell];
        }

        // Parse formula
        Tokens addend_tokens = split(tokens[1], '+');
        Formula f;
        for (const auto& it : addend_tokens) {
            if (validate_cell(it)) {

                int next_id;
                if (ids.find(it) == ids.end()) {
                    next_id = ids.size();
                    ids[it] = ids.size();
                } else {
                    next_id = ids[it];
                }

                f.push_back(AddendFactory::CellAddend(next_id));
            } else {
                try {
                    f.push_back(AddendFactory::ValueAddend(std::stoi(it)));
                }
                catch (std::exception& e) {
                    throw ParserException("Addend " + it + " is not cell or value");
                }
            }
        }

        InputCellInfo info;

        info.id = id;
        info.name = cell;
        info.formula = f;
        
        input_data.push_back(info);
        //input_data[input_data_id++] = CellInfoIO(id, cell, f);
        line_num++;
    }
    return input_data;
}

