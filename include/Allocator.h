#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <vector>
#include <string>
#include "Web.h"
#include "Parser.h"
#include "Graph.h"

/**
 * @brief The brain of our compiler backend tool.
 *
 * @details This class takes the parsed webs and config, builds the interference graph,
 * and tries to assign physical registers to each web without overlap.
 */
class RegisterAllocator {
private:
    std::vector<Web> webs;
    Config config;

    // our interference graph. the 'int' template parameter will just hold the web id.
    Graph<int> interferenceGraph;

    /**
     * @brief the core greedy coloring algorithm described in the assignment.
     *
     * @param numRegisters the maximum number of colors (registers) we can use.
     * @return true if we successfully colored the graph, false if we failed (need to spill/split).
     *
     * @note Time Complexity: O(V + E) where V is the number of webs and E is the number of interferences.
     */
    bool colorGraph(int numRegisters);

    /**
     * @brief Populates the interference graph based on the webs' live ranges.
     *
     * @details it loops through all webs and adds bidirectional edges if they interfere.
     */
    void buildInterferenceGraph();

public:
    /**
     * @brief sets up the allocator with the parsed data.
     */
    RegisterAllocator(const std::vector<Web>& parsedWebs, const Config& parsedConfig);

    /**
     * @brief executes the basic allocation (T2.1).
     * @details tries to color the graph. if it fails, webs get mapped to memory ('M').
     */
    void runBasicAllocation();

    /**
     * @brief executes allocation with web spilling (T2.2).
     * @details if basic coloring fails, we pick 'k' webs to remove from the graph and send to memory.
     */
    void runSpillingAllocation();

    /**
     * @brief executes allocation with web splitting (T2.3).
     * @details if coloring fails, we try slicing 'k' webs into smaller pieces to break interferences.
     */
    void runSplittingAllocation();

    /**
     * @brief executes the free-style allocation (T2.4).
     */
    void runFreeAllocation();

    /**
     * @brief exports the final register mappings to the output text file.
     *
     * @param filename the path to the output file (e.g., allocation.txt).
     */
    void generateOutput(const std::string& filename);
};

#endif