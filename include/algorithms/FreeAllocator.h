#ifndef FREE_ALLOCATOR_H
#define FREE_ALLOCATOR_H

#include "algorithms/AllocationStrategy.h"

/**
 * @brief Strategy for free/custom register allocation (T2.4).
 */
class FreeAllocator : public AllocationStrategy {
public:
    /**
     * @brief Executes a hybrid heuristic combining aggressive spilling with optimistic coloring.
     * @timecomplexity O(V log V + V * (V + E))
     */
    void allocate(std::vector<Web>& webs, const Config& config, Graph<int>& interferenceGraph) override;
};

#endif
