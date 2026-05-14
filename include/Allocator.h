#ifndef ALLOCATOR_H
#define ALLOCATION_H

#include <vector>
#include <string>
#include <memory>
#include "Web.h"
#include "Parser.h"
#include "Graph.h"
#include "algorithms/AllocationStrategy.h"

/**
 * @brief The Context class for our compiler backend tool.
 *
 * @details This class delegates the allocation algorithms using the Strategy Pattern.
 */
class RegisterAllocator {
private:
    std::vector<Web> webs;
    Config config;
    Graph<int> interferenceGraph;

    /**
     * @brief Populates the interference graph based on the webs' live ranges.
     * @timecomplexity O(V^2 * L)
     */
    void buildInterferenceGraph();

public:
    /**
     * @brief sets up the allocator with the parsed data.
     */
    RegisterAllocator(const std::vector<Web>& parsedWebs, const Config& parsedConfig);

    /**
     * @brief executes the basic allocation (T2.1).
     * @timecomplexity O(V * (V + E)) bounded by the coloring step.
     */
    void runBasicAllocation();

    /**
     * @brief executes allocation with web spilling (T2.2).
     * @timecomplexity O(V * (V + E)) bounded by the coloring step with spills.
     */
    void runSpillingAllocation();

    /**
     * @brief executes allocation with web splitting (T2.3).
     * @timecomplexity O(K * V * (V + E)) where K is the number of max splits allowed.
     */
    void runSplittingAllocation();

    /**
     * @brief executes the free-style allocation (T2.4).
     * @timecomplexity O(V log V + V * (V + E))
     */
    void runFreeAllocation();

    /**
     * @brief exports the final register mappings to the output text file.
     * @timecomplexity O(V * L) where V is the number of webs and L is the number of lines.
     */
    void generateOutput(const std::string& filename);
};

#endif