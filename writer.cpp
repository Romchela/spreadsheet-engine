#include <fstream>
#include "writer.h"

void Writer::write(const OutputData& output_data, const std::string& output_file_path) {
    using CellResult = std::pair<std::string, ValueType>;
    std::vector<CellResult> v(output_data.size());
    int pos = 0;
    for (const auto& it : output_data) {
        v[pos] = it;
        pos++;
    }
    
    sort(v.begin(), v.end(), [](const CellResult& cell_a, const CellResult& cell_b) {
        const std::string& a = cell_a.first;
        const std::string& b = cell_b.first;
        if (a[0] < b[0]) {
            return 1;
        } else if (a[0] > b[0]) {
            return 0;
        } else {
            int a_int = std::stoi(a.substr(1, a.length() - 1));
            int b_int = std::stoi(b.substr(1, b.length() - 1));
            if (a_int == b_int) {
                return (int)(cell_a.second < cell_b.second);
            }
            return (int)(a_int < b_int);
        }
    });

    std::ofstream output_file;
    output_file.open(output_file_path);
    for (const auto& it : v) {
        output_file << it.first << " = " << it.second << std::endl;
    }
    output_file.close();
}

