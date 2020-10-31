#ifndef SPREADSHEETENGINE_IO_DATA_H
#define SPREADSHEETENGINE_IO_DATA_H

#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>
#include <string>

using ValueType = int;
#define to_value_type(x) (ValueType) std::stoi(x);

struct Addend {
    Addend() = default;

    enum Type {
        CELL,
        VALUE
    };

    Type type;
    std::string value;
};

class AddendFactory {
public:
    static Addend CellAddend(const std::string& c) {
        Addend addend = Addend();
        addend.type = Addend::Type::CELL;
        addend.value = c;
        return addend;
    }

    static Addend ValueAddend(const std::string& v) {
        Addend addend = Addend();
        addend.type = Addend::Type::VALUE;
        addend.value = (v);
        return addend;
    }
};


struct Formula {
    std::vector<Addend> addends;
};

using InputData = std::unordered_map<std::string, Formula>;
using OutputData = std::unordered_map<std::string, ValueType>;

#endif //SPREADSHEETENGINE_IO_DATA_H