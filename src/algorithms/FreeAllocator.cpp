#include "algorithms/FreeAllocator.h"
#include <map>
#include <set>

void FreeAllocator::allocate(std::vector<Web>& webs, const Config& config, Graph<int>& interferenceGraph) {
    int maxPossiblesSpills = webs.size();

    if (!colorGraph(interferenceGraph, config.maxRegisters, maxPossiblesSpills, webs)) {
        for (auto& web : webs) { web.assignedRegister = -1; }
    } else {
        std::map<int, Web*> webMap;
        for (auto& web : webs) {
            webMap[web.id] = &web;
        }

        for (auto& web : webs) {
            if (web.assignedRegister == -1) {
                std::set<int> takenColors;
                auto* vertex = interferenceGraph.findVertex(web.id);
                if (vertex != nullptr) {
                    for (auto* edge : vertex->getAdj()) {
                        int neighborId = edge->getDest()->getInfo();
                        if (webMap.find(neighborId) != webMap.end() && webMap[neighborId]->assignedRegister != -1) {
                            takenColors.insert(webMap[neighborId]->assignedRegister);
                        }
                    }
                }
                
                for (int color = 0; color < config.maxRegisters; ++color) {
                    if (takenColors.find(color) == takenColors.end()) {
                        web.assignedRegister = color;
                        break;
                    }
                }
            }
        }
    }
}
