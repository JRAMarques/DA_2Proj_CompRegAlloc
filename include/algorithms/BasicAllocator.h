#ifndef BASIC_ALLOCATOR_H
#define BASIC_ALLOCATOR_H

#include "algorithms/AllocationStrategy.h"

/**
 * @brief Strategy for basic register allocation (T2.1).
 */
class BasicAllocator : public AllocationStrategy {
public:
    /**
     * @brief Executes basic allocation. If it fails, all webs go to memory.
     * @timecomplexity O(V * (V + E))
     */
    void allocate(std::vector<Web>& webs, const Config& config, Graph<int>& interferenceGraph) override;
};

#endif
