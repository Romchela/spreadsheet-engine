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
    
    enum Type {
        CELL,
        VALUE
    };

    Addend() = default;
    Addend(Type type, int value) : type(type), value(value) {}

    Type type;
    int value;
};

class AddendFactory {
public:
    static Addend CellAddend(int c) {
        Addend addend = Addend();
        addend.type = Addend::Type::CELL;
        addend.value = c;
        return addend;
    }

    static Addend ValueAddend(int v) {
        Addend addend = Addend();
        addend.type = Addend::Type::VALUE;
        addend.value = v;
        return addend;
    }
};

using Formula = std::vector<Addend>;

struct InputCellInfo {
    int id;
    std::string name;
    Formula formula;

    InputCellInfo() = default;
    InputCellInfo(int id, std::string & name, Formula & formula) : id(id), name(name), formula(formula) {};
};

using InputData = std::vector<InputCellInfo>;
using OutputData = std::unordered_map<std::string, ValueType>;

#endif //SPREADSHEETENGINE_IO_DATA_H
