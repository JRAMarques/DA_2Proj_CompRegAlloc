#include "Allocator.h"
#include "io/OutputGenerator.h"
#include "algorithms/BasicAllocator.h"
#include "algorithms/SpillingAllocator.h"
#include "algorithms/SplittingAllocator.h"
#include "algorithms/FreeAllocator.h"
#include <iostream>

RegisterAllocator::RegisterAllocator(const std::vector<Web>& parsedWebs, const Config& parsedConfig)
    : webs(parsedWebs), config(parsedConfig) {}

void RegisterAllocator::buildInterferenceGraph() {
    for (const auto& web : webs) {
        interferenceGraph.addVertex(web.id);
    }

    for (size_t i = 0; i < webs.size(); ++i) {
        for (size_t j = i + 1; j < webs.size(); ++j) {
            if (webs[i].interferesWith(webs[j])) {
                interferenceGraph.addBidirectionalEdge(webs[i].id, webs[j].id, 1.0);
            }
        }
    }
}

void RegisterAllocator::runBasicAllocation() {
    buildInterferenceGraph();
    BasicAllocator strategy;
    strategy.allocate(webs, config, interferenceGraph);
}

void RegisterAllocator::runSpillingAllocation() {
    buildInterferenceGraph();
    SpillingAllocator strategy;
    strategy.allocate(webs, config, interferenceGraph);
}

void RegisterAllocator::runSplittingAllocation() {
    buildInterferenceGraph();
    SplittingAllocator strategy;
    strategy.allocate(webs, config, interferenceGraph);
}

void RegisterAllocator::runFreeAllocation() {
    buildInterferenceGraph();
    FreeAllocator strategy;
    strategy.allocate(webs, config, interferenceGraph);
}

void RegisterAllocator::generateOutput(const std::string& filename) {
    OutputGenerator::generate(filename, webs, config);
}