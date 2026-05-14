#include "algorithms/BasicAllocator.h"
#include <iostream>

void BasicAllocator::allocate(std::vector<Web>& webs, const Config& config, Graph<int>& interferenceGraph) {
    if (!colorGraph(interferenceGraph, config.maxRegisters, 0, webs)) {
        std::cerr << "Warning: Basic allocation failed. Not enough registers. Moving all the variables to memory.\n";
        for (auto& web : webs) {
            web.assignedRegister = -1;
        }
    }
}
