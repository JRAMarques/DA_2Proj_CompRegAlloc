#ifndef SPILLING_ALLOCATOR_H
#define SPILLING_ALLOCATOR_H

#include "algorithms/AllocationStrategy.h"

/**
 * @brief Strategy for spilling register allocation (T2.2).
 */
class SpillingAllocator : public AllocationStrategy {
public:
    /**
     * @brief Executes allocation allowing a configured number of spills.
     * @timecomplexity O(V * (V + E))
     */
    void allocate(std::vector<Web>& webs, const Config& config, Graph<int>& interferenceGraph) override;
};

#endif
