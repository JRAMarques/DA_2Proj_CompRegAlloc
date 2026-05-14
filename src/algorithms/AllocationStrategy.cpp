#include "algorithms/AllocationStrategy.h"
#include <map>
#include <set>
#include <algorithm>

bool AllocationStrategy::drainGraphToStack(Graph<int>& interferenceGraph, int numRegisters, int maxSpills, std::stack<int>& s, std::vector<Web>& webs) {
    int spillsUsed = 0;

    std::map<int, std::set<int>> adjList;
    for (auto* vertex : interferenceGraph.getVertexSet()) {
        int id = vertex->getInfo();
        adjList[id];
        for (auto* edge : vertex->getAdj()) {
            adjList[id].insert(edge->getDest()->getInfo());
        }
    }

    while (!adjList.empty()) {
        bool foundNodeToRemove = false;

        for (auto it = adjList.begin(); it != adjList.end(); ++it) {
            int nodeId = it->first;
            int degree = it->second.size();

            if (degree < numRegisters) {
                s.push(nodeId);
                for (int neighbor : it->second) {
                    adjList[neighbor].erase(nodeId);
                }
                adjList.erase(it);
                foundNodeToRemove = true;
                break;
            }
        }

        if (!foundNodeToRemove && !adjList.empty()) {
            if (spillsUsed < maxSpills) {
                auto maxIt = adjList.begin();
                for (auto it = adjList.begin(); it != adjList.end(); ++it) {
                    if (it->second.size() > maxIt->second.size()) {
                        maxIt = it;
                    }
                }

                int nodeToSpill = maxIt->first;

                for (int neighbor : maxIt->second) {
                    adjList[neighbor].erase(nodeToSpill);
                }
                adjList.erase(maxIt);

                auto itWeb = std::find_if(webs.begin(), webs.end(),
                                          [nodeToSpill](const Web& w){ return w.id == nodeToSpill; });
                if (itWeb != webs.end()) {
                    itWeb->assignedRegister = -1;
                }
                spillsUsed++;
            } else {
                return false;
            }
        }
    }
    return true;
}

void AllocationStrategy::assignColors(Graph<int>& interferenceGraph, int numRegisters, std::stack<int>& s, std::vector<Web>& webs) {
    std::map<int, Web*> webMap;
    for (auto& web : webs) {
        webMap[web.id] = &web;
    }

    while (!s.empty()) {
        int nodeId = s.top();
        s.pop();

        std::set<int> takenColors;
        auto* vertex = interferenceGraph.findVertex(nodeId);

        if (vertex == nullptr) continue;

        for (auto* edge : vertex->getAdj()) {
            int neighborId = edge->getDest()->getInfo();
            if (webMap.find(neighborId) != webMap.end()) {
                int neighborColor = webMap[neighborId]->assignedRegister;
                if (neighborColor != -1) {
                    takenColors.insert(neighborColor);
                }
            }
        }

        for (int color = 0; color < numRegisters; ++color) {
            if (takenColors.find(color) == takenColors.end()) {
                if (webMap.find(nodeId) != webMap.end()) {
                    webMap[nodeId]->assignedRegister = color;
                }
                break;
            }
        }
    }
}

bool AllocationStrategy::colorGraph(Graph<int>& interferenceGraph, int numRegisters, int maxSpills, std::vector<Web>& webs) {
    std::stack<int> s;

    if (!drainGraphToStack(interferenceGraph, numRegisters, maxSpills, s, webs)) {
        return false;
    }

    assignColors(interferenceGraph, numRegisters, s, webs);

    return true;
}

void AllocationStrategy::rebuildInterferenceGraph(Graph<int>& interferenceGraph, const std::vector<Web>& webs) {
    // Clear old vertices
    std::vector<int> nodesToRemove;
    for (auto* vertex : interferenceGraph.getVertexSet()) {
        nodesToRemove.push_back(vertex->getInfo());
    }
    for (int id : nodesToRemove) {
        interferenceGraph.removeVertex(id);
    }

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
