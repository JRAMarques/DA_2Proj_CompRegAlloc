#include "algorithms/SpillingAllocator.h"
#include <iostream>

void SpillingAllocator::allocate(std::vector<Web>& webs, const Config& config, Graph<int>& interferenceGraph) {
    if (!colorGraph(interferenceGraph, config.maxRegisters, config.algoParameter, webs)) {
        std::cerr << "Warning: the allocation failed using "
                  << config.algoParameter << " spills. Moving all the variables to memory.\n";
        for (auto& web : webs) {
            web.assignedRegister = -1;
        }
    }
}
