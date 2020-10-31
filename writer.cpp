
#include <fstream>
#include "writer.h"

void Writer::write(const OutputData& output_data, const std::string& output_file_path) {
    std::vector<std::pair<std::string, ValueType>> v(output_data.size());
    int pos = 0;
    for (const auto& it : output_data) {
        v[pos] = it;
        pos++;
    }
    sort(v.begin(), v.end());

    std::ofstream output_file;
    output_file.open(output_file_path);
    for (const auto& it : v) {
        output_file << it.first << " = " << it.second << std::endl;
    }
    output_file.close();
}

