#include "algorithms/SplittingAllocator.h"
#include <iostream>

bool SplittingAllocator::attemptWebSplit(std::vector<Web>& webs) {
    int bestWebIndex = -1;
    int maxHoleSize = 0;
    int bestSplitPoint = -1;

    for (size_t i = 0; i < webs.size(); ++i) {
        auto holeInfo = webs[i].findBiggestHole();
        int holeSize = holeInfo.first;
        int splitPoint = holeInfo.second;

        if (holeSize > maxHoleSize) {
            maxHoleSize = holeSize;
            bestSplitPoint = splitPoint;
            bestWebIndex = static_cast<int>(i);
        }
    }

    if (bestWebIndex == -1 || maxHoleSize <= 1) {
        return false;
    }

    Web targetWeb = webs[bestWebIndex];

    int maxId = 0;
    for (const auto& w : webs) {
        if (w.id > maxId) {
            maxId = w.id;
        }
    }

    auto newWebs = targetWeb.split(maxId + 1, maxId + 2, bestSplitPoint);

    webs.erase(webs.begin() + bestWebIndex);
    webs.push_back(newWebs.first);
    webs.push_back(newWebs.second);
    
    return true;
}

void SplittingAllocator::allocate(std::vector<Web>& webs, const Config& config, Graph<int>& interferenceGraph) {
    int maxSplits = config.algoParameter;
    int splitsPerformed = 0;
    bool success = false;

    success = colorGraph(interferenceGraph, config.maxRegisters, 0, webs);

    while (!success && splitsPerformed < maxSplits) {
        if (!attemptWebSplit(webs)) {
            break;
        }

        splitsPerformed++;

        rebuildInterferenceGraph(interferenceGraph, webs);
        success = colorGraph(interferenceGraph, config.maxRegisters, 0, webs);
    }

    if (!success) {
        std::cerr << "Warning: the Splitting Allocation has consumed " << splitsPerformed
                  << " splittings and still misses registers. The remaining variables are sent to memory.\n";
        for (auto& web : webs) {
            if (web.assignedRegister == -1) {
                web.assignedRegister = -1;
            }
        }
    }
}
