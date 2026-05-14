#ifndef SPLITTING_ALLOCATOR_H
#define SPLITTING_ALLOCATOR_H

#include "algorithms/AllocationStrategy.h"

/**
 * @brief Strategy for splitting register allocation (T2.3).
 */
class SplittingAllocator : public AllocationStrategy {
private:
    /**
     * @brief Helper to find the best web to split and perform the split.
     * @timecomplexity O(V * L)
     */
    bool attemptWebSplit(std::vector<Web>& webs);

public:
    /**
     * @brief Executes allocation by splitting webs when basic coloring fails.
     * @timecomplexity O(K * V * (V + E)) where K is the number of max splits allowed.
     */
    void allocate(std::vector<Web>& webs, const Config& config, Graph<int>& interferenceGraph) override;
};

#endif
