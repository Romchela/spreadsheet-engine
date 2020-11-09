cd spreadsheet-engine
dos2unix inputs/*
dos2unix outputs/*
dos2unix ../run.sh
g++-9 -O2 -std=c++17 -I/usr/local/Cellar/tbb/2020_U3_1/include/ -L/usr/local/Cellar/tbb/2020_U3_1/lib/ -ltbb engine.cpp reader.cpp writer.cpp solutions/fast.cpp solutions/one-thread-simple.cpp -o ../engine.out
