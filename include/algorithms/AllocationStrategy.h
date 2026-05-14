#ifndef ALLOCATION_STRATEGY_H
#define ALLOCATION_STRATEGY_H

#include <vector>
#include <stack>
#include "Web.h"
#include "Parser.h"
#include "Graph.h"

/**
 * @brief Abstract base class for all allocation strategies.
 * Provides common graph coloring utilities (Strategy Pattern).
 */
class AllocationStrategy {
protected:
    /**
     * @brief Drains nodes to a stack, performing spills if necessary.
     * @timecomplexity O(V * (V + E))
     */
    bool drainGraphToStack(Graph<int>& interferenceGraph, int numRegisters, int maxSpills, std::stack<int>& s, std::vector<Web>& webs);

    /**
     * @brief Assigns colors to nodes from the stack.
     * @timecomplexity O(V * D)
     */
    void assignColors(Graph<int>& interferenceGraph, int numRegisters, std::stack<int>& s, std::vector<Web>& webs);

    /**
     * @brief Attempts to color the graph using greedy heuristic.
     * @timecomplexity O(V * (V + E))
     */
    bool colorGraph(Graph<int>& interferenceGraph, int numRegisters, int maxSpills, std::vector<Web>& webs);

    /**
     * @brief Rebuilds the interference graph from a set of webs.
     * @timecomplexity O(V^2 * L)
     */
    void rebuildInterferenceGraph(Graph<int>& interferenceGraph, const std::vector<Web>& webs);

public:
    virtual ~AllocationStrategy() = default;

    /**
     * @brief Main method to execute the allocation.
     * @param webs The variables and their ranges.
     * @param config The settings.
     * @param interferenceGraph The graph of interferences.
     */
    virtual void allocate(std::vector<Web>& webs, const Config& config, Graph<int>& interferenceGraph) = 0;
};

#endif
