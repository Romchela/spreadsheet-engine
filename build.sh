cd spreadsheet-engine
g++-9 -O2 -std=c++17 -ltbb engine.cpp reader.cpp writer.cpp solutions/fast.cpp solutions/one-thread-simple.cpp -o ../engine.out
