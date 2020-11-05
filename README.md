# Spreadsheet computation engine

## Problem description
A big project for the computation team is to build a spreadsheet computation engine. The engine is written in a system language (C++ or Rust) and is embedded both into Frontend (via WebAssembly), native platforms (desktop, mobile) and Backend.

The task is to write a toy computation algorithm that parses the input file (input.txt), calculates the values of all of the cells and outputs their values into the output file.

There is a high potential for parallelism in this task. **It is required to implement an algorithm that spreads calculation across multiple threads.** This parallelization is the main algorithm that is implemented. It's not that trivial to implement, but this is what makes this task interesting. The main use case to optimize for - the user loads the data once (and doesn't care much about the load time, within reason)  and then frequently edits various cells, which triggers updates to the parts of the graph.  

Any code that is not related to the graph/computation algorithm is written in the easiest possible way.

To simplify implementation all of the cells are either values or sums of other values. However, this fact should not be used (e.g. linearity of output) during the implementation / optimizations. The fact that these are simple sums means that we might not even see enough benefit from parallelization. So, we can add occasional random X ms delays to the summation function to see at what X the runtime benefits from parallelization.

Input file: [link](https://drive.google.com/file/d/15DBou3JBA-E-51npnyVy420q_8ZbMa-y/view?usp=sharing) 

Output file contains one cell/value per row (in no particular order) e.g.

    A17 = 8
    A19 = 2048
    ...
    Z67 = 3818

## Implementation

The program receives 5 arguments:

1) File path to the data which is loaded once in the format described below e.g. `inputs/2_initial.txt`
2) File path to the data which contains edits of various cells. File format is the same but we do updates cell by cell. This file contains cell modifications which have a small total number of dependencies (not many cells depend on this cell value) e.g. `inputs/2_modifications_small.txt`
3) Same as the second argument, but there are cell modifications which have a medium total dependency count e.g. `inputs/2_modifications_medium.txt`
4) Same as the third argument, but there are cell modifications which have a large total dependency count e.g. `inputs/2_modifications_large.txt`
5) Path to the folder where the answers will be stored e.g. `outputs/`

Program entry point is `int main()` method in `engine.cpp`. It runs multiple solutions, calculates the time of execution and compares all the results. Four output files are generated for each solution: values of all cells after initial data load, values after all cell modifications with a small total dependency count, values after all medium modifications and after all large modifications.

One of the solution uses lock-free queue implementation from https://github.com/cameron314/concurrentqueue

    
### Tests

1) Small test for debug purposes

    `inputs/1_initial.txt`

    `inputs/1_modifications_small.txt`

    `inputs/1_modifications_medium.txt`

    `inputs/1_modifications_large.txt` 


2) Large test

    `inputs/2_initial.txt` - input from problem statement
    
    `inputs/2_modifications_small.txt` - cell changing where total dependency count < 10
    
    `inputs/2_modifications_medium.txt` - cell changing where total dependency count ~= 2000
    
    `inputs/2_modifications_large.txt` - cell changing (max top three cells by total dependency count).

### Solutions

Each solution implements an interface containing 3 methods: 

1) InitialCalculate - calculates all cell values (initial  data loading).
2) ChangeCell - edits the cell value and recalculates the state.
3) GetCurrentValues - returns values of all cells (nothing interesting, simple construction of OutputData for writing it into a file)

### 1. Single thread simple solution

**Files:** solutions/one-thread-simple.cpp, solutions/one-thread-simple.h

**Overall description:** efficient solution in linear time and space complexity but it's dificult to parallelize it.

#### InitialCalculate method:

Go through initial data and calculate cell values. If we calculate the value of cell `A` and its formula contains cell `B` we calculate the value of `B` recursively. If value of the cell is already calculated we don't need to do it twice and stop the recursion. Also we simply build dependency graph - an edge `A -> B` exists if and ony if formula of `B` contains `A`. It is required for ChangeCell method.

**Time complexity:** O(n), n - number of cells.

**Space complexity:** O(n)

#### ChangeCell method:

We want to change value of cell `A` First, we recalculate the dependency graph (we need to change edges only for direct children). Then, build toplogical sort of subgraph which contains all reachable nodes from `A` by dependency graph. And finally go through subgraph in order of top sort and recalculate the values of cells. 

**Time complexity:** O(g), g - number of cells reachable from `A`

**Space complexity:** O(g)

### 2. Fast solution

**Files:** solutions/fast.cpp, solutions/fast.h

**Overall description:** efficient solution in linear time and space complexity based on breadth-first search which can be parallelized. We need to use special thread-safe collections which allow efficient parallel access/modifications.

#### InitialCalculate method:

Parallel DAG building (directed acyclic graph) - the same as dependency graph in single thread simple solution. Then, calculate values of all cells in parallel: push cells that do not depend on any other cells to the queue  as a starting state, calculate their values and push all adjacent cells to the queue and so on.

**Time complexity:** O(n), n - number of cells.

**Space complexity:** O(n)

#### ChangeCell method:

We change the cell `A`. First, build an array of cells reachable from `A` in parallel by DAG. Then, recalulate values of these cells. We don't have top sort here, so the algorithm is a bit different. Push `A` to the queue as a starting state. Repeat while queue is not empty: get a cell from the queue and check if all cells in the formula are recalculated. If yes, we can recalculate the value and push all adjacent nodes, otherwise we push `A` one more time to the queue and start a new iteration.

**Time complexity:** O(g), g - number of cells reachable from `A`.

**Space complexity:** O(g)

## Requirements

C++17, visual studio, windows x64.

## Benchmark

**Version 0.2**

AMD Ryzen 7 3700 X 8-Core Processor 3.59 Ghz 16 GB RAM **x86**.
Fast solution uses **16** threads.

![](benchmarks/benchmark4.jpg)

1) InitialCalculate OneThreadSimple `1788 ms` vs Fast `287 ms`. Fast solution **6.2 times faster**.
2) ChangeCell for medium and small works quite fast (both solutions) `< 10 ms`.
3) ChangeCell for large OneThreadSimple `658 ms` vs Fast `260 ms`. Fast solution **2.5 times faster**.

AMD Ryzen 7 3700 X 8-Core Processor 3.59 Ghz 16 GB RAM **x64**.
Fast solution uses **16** threads.

![](benchmarks/benchmark3.jpg)

1) InitialCalculate OneThreadSimple `2003 ms` vs Fast `328 ms`. Fast solution **6.1 times faster**.
2) ChangeCell for medium and small works quite fast (both solutions) `< 10 ms`.
3) ChangeCell for large OneThreadSimple `682 ms` vs Fast `290 ms`. Fast solution **2.3 times faster**.

Intel(R) Core(TM) i7-8550U CPU @ 1.80GHz 1.99 GHz RAM 16.0 GB x64.
Fast solution uses 8 threads.

![](benchmarks/benchmark2.PNG)

1) InitialCalculate OneThreadSimple `2765 ms` vs Fast `690 ms`. Fast solution **4 times faster**.
2) ChangeCell for medium and small works quite fast (both solutions) `~20 ms`.
3) ChangeCell for large OneThreadSimple `882 ms` vs Fast `686 ms`. Fast solution **1.3 times faster**.

**Version 0.1**

Intel(R) Core(TM) i7-8550U CPU @ 1.80GHz 1.99 GHz RAM 16.0 GB x64.
Fast solution uses 8 threads.


![](benchmarks/benchmark1.png)

1) InitialCalculate OneThreadSimple `8813 ms` vs Fast `3094 ms`. Fast solution **2.8 times faster**.
2) ChangeCell for medium and small works quite fast (both solutions) `~20 ms`.
3) ChangeCell for large OneThreadSimple `5801 ms` vs Fast `3886 ms`. Fast solution **1.5 times faster**.

## TODO and possible optimizations

1) Support cycles. Formulas can be invalid and DAG becomes cyclic. We need to detect it and return error as value for all cells on a cycle.
2) ~~Currently cells are identified by their string name, we can map string -> int and use int everywhere instead of string. It can increase performance of hash maps.~~
3) [Fast solution] After cells are changed, we need to recalulate DAG and some edges shoud be deleted. Unfortunately, concurrent_unordered_map doesn't support deletion, so we just mark the edge as removed. After a lot of modifications we can store a lot of useless deleted edges, so we need to implement a background job which will periodically rebuild DAG (explicitly remove unused edges).
4) ~~[Fast solution] If we call ChangeCell method for a cell which has total dependency count of around 99% nodes, it will work a bit slower than if we had called InitialCalculate and built the whole graph from ground up. So, we need to store the total dependency count for each cell and choose how to update cell value depending on that value.~~
5) ~~[Fast solution] Profiling shows a thing that is expected: much of the performance depends on concurrent data structure implementations. We can try different implementations to choose a better one.~~
6) Optimize IO (std::ifstream, std::ofstream slow?)
7) Version for linux/macOS.
