#ifndef SPREADSHEETENGINE_WRITER_H
#define SPREADSHEETENGINE_WRITER_H

#include <iostream>
#include "io-data.h"

class Writer {
public:
    static void write(const OutputData& output_data, const std::string& output_file_path);
};

#endif //SPREADSHEETENGINE_WRITER_H
