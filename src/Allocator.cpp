#include "Allocator.h"
#include <iostream>
#include <fstream>
#include <stack>
#include <map>
#include <algorithm>

// receives the merged webs from the parser and stores everything internally.
RegisterAllocator::RegisterAllocator(const std::vector<Web>& parsedWebs, const Config& parsedConfig)
    : webs(parsedWebs), config(parsedConfig) {}


// turns the web vector into an interference graph.
// each web becomes a vertex, and if two webs are live at the same time,
// we add a bidirectional edge between them — they can't share a register.
void RegisterAllocator::buildInterferenceGraph() {
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

// greedy graph coloring algorithm (figure 9 of the spec).
// phase 1: find nodes with degree < n, remove them from the temp graph and push to stack.
// if we get stuck (all nodes have degree >= n), return false.
// phase 2: pop from stack and assign the first color that no neighbor is using.
bool RegisterAllocator::colorGraph(int numRegisters, int maxSpills) {
    std::stack<int> s;
    int spillsUsed = 0;

    // Copy of the adjency list to manipulate during step 1
    std::map<int, std::set<int>> adjList;
    for (auto* vertex : interferenceGraph.getVertexSet()) {
        int id = vertex->getInfo();
        adjList[id];
        for (auto* edge : vertex->getAdj()) {
            adjList[id].insert(edge->getDest()->getInfo());
        }
    }

    //Step 1: Sending from the stack to the graph
    while (!adjList.empty()) {
        bool foundNodeToRemove = false;

        for (auto it = adjList.begin(); it != adjList.end(); ++it) {
            int nodeId = it->first;
            int degree = it->second.size();

            // Nodes with degree inferior to available registers are sent to the stack
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

        // Deadlock: Every node has degree >= numRegisters
        if (!foundNodeToRemove && !adjList.empty()) {
            if (spillsUsed < maxSpills) {
                // Spilling Heuristic: Choosing the node with the highest degree
                auto maxIt = adjList.begin();
                for (auto it = adjList.begin(); it != adjList.end(); ++it) {
                    if (it->second.size() > maxIt->second.size()) {
                        maxIt = it;
                    }
                }

                int nodeToSpill = maxIt->first;

                //delete the node from the lists
                for (int neighbor : maxIt->second) {
                    adjList[neighbor].erase(nodeToSpill);
                }
                adjList.erase(maxIt); //removed from the temporary graph

                //removed node doesn't enter the stack and is marked (-1)
                auto itWeb = std::find_if(webs.begin(), webs.end(),
                                          [nodeToSpill](const Web& w){ return w.id == nodeToSpill; });
                if (itWeb != webs.end()) {
                    itWeb->assignedRegister = -1;
                }
                spillsUsed++;
            } else {
                //fails at coloring after exceeding spill k trials
                return false;
            }
        }
    }

    // Step 2: Assigning colors to registers
    std::map<int, Web*> webMap;
    for (auto& web : webs) {
        webMap[web.id] = &web;
    }

    //take the nodes from the stack (FIFO)
    while (!s.empty()) {
        int nodeId = s.top();
        s.pop();

        std::set<int> takenColors; // Colors taken by the neighborhood
        auto* vertex = interferenceGraph.findVertex(nodeId);

        // checking neighbor colors
        for (auto* edge : vertex->getAdj()) {
            int neighborId = edge->getDest()->getInfo();
            int neighborColor = webMap[neighborId]->assignedRegister;

            // If the neighbor already has a color (not -1), we save it as taken
            if (neighborColor != -1) {
                takenColors.insert(neighborColor);
            }
        }

        // Assign the first free color (registo)
        for (int color = 0; color < numRegisters; ++color) {
            if (takenColors.find(color) == takenColors.end()) {
                webMap[nodeId]->assignedRegister = color;
                break; // Color is assigned
            }
        }
    }

    return true;
}

// builds the graph and tries to color it. if it fails, all webs go to memory (m).
void RegisterAllocator::runBasicAllocation() {
    buildInterferenceGraph();

    if (!colorGraph(config.maxRegisters,0)) {
        std::cerr << "Warning: Basic allocation failed. Not enough registers. Moving all the variables to memory.\n";
        for (auto& web : webs) {
            web.assignedRegister = -1; // -1 means memory
        }
    }
}


void RegisterAllocator::runSpillingAllocation() {
    buildInterferenceGraph();

    // In the Spilling Allocation, we let the algorithm throwing 'K' webs
    if (!colorGraph(config.maxRegisters, config.algoParameter)) {
        std::cerr << "Warning: the allocation failed using "
                  << config.algoParameter << " spills. Moving all the variables to memory.\n";
        for (auto& web : webs) {
            web.assignedRegister = -1;
        }
    }
}

void RegisterAllocator::runSplittingAllocation() {
    int maxSplits = config.algoParameter;
    int splitsPerformed = 0;
    bool success = false;

    // Initial coloring with no splitting
    buildInterferenceGraph();
    success = colorGraph(config.maxRegisters, 0);

    // Iteratively splits
    while (!success && splitsPerformed < maxSplits) {
        int bestWebIndex = -1;
        int maxHoleSize = 0;
        int bestSplitPoint = -1;

        // Searches the web with the largest hole
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

        // Stop condition: breaks if there are no holes to splitting
        if (bestWebIndex == -1 || maxHoleSize <= 1) {
            break;
        }

        // Extracting the data needed for the splitting operation
        Web targetWeb = webs[bestWebIndex];

        // Determine the next available IDs to split
        int maxId = 0;
        for (const auto& w : webs) {
            if (w.id > maxId) {
                maxId = w.id;
            }
        }

        // Executing the physical split
        auto newWebs = targetWeb.split(maxId + 1, maxId + 2, bestSplitPoint);

        // Substituting the original web by the fractions
        webs.erase(webs.begin() + bestWebIndex);
        webs.push_back(newWebs.first);
        webs.push_back(newWebs.second);

        splitsPerformed++;

        // Cleaning the old interference graph vertices before reconstructing
        std::vector<int> nodesToRemove;
        for (auto* vertex : interferenceGraph.getVertexSet()) {
            nodesToRemove.push_back(vertex->getInfo());
        }
        for (int id : nodesToRemove) {
            interferenceGraph.removeVertex(id);
        }

        // Reconstructing the graph
        buildInterferenceGraph();
        success = colorGraph(config.maxRegisters, 0);
    }

    // If the coloring is still impossible the non-allocated variables are sent to memory
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
void RegisterAllocator::runFreeAllocation() {
    buildInterferenceGraph();

    std::sort(webs.begin(), webs.end(), [](const Web& a, const Web& b) {
        return (a.useLines.size() + a.defLines.size()) > (b.useLines.size() + b.defLines.size());
    });

    if (!colorGraph(config.maxRegisters, config.maxRegisters)) {
        for (auto& web : webs) { web.assignedRegister = -1; }
    }
}


// writes the output file in the exact format required by the spec.
void RegisterAllocator::generateOutput(const std::string& filename) {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Error: Could not create output file.\n";
        return;
    }

    out << "webs: " << webs.size() << "\n";
    for (const auto& web : webs) {
        out << "web" << web.id << ": ";

        bool first = true;
        for (int line : web.activeLines) {
            if (!first) out << ",";
            out << line;

            if (web.defLines.find(line) != web.defLines.end()) out << "+";
            else if (web.useLines.find(line) != web.useLines.end()) out << "-";

            first = false;
        }
        out << "\n";
    }

    // if every web ended up in memory, the number of used registers is 0
    bool failed = true;
    for (const auto& web : webs) {
        if (web.assignedRegister != -1) {
            failed = false;
            break;
        }
    }

    if (failed) {
        out << "registers: 0\n";
    } else {
        out << "registers: " << config.maxRegisters << "\n";
    }

    for (const auto& web : webs) {
        if (web.assignedRegister == -1) {
            out << "M: web" << web.id << "\n";
        } else {
            out << "r" << web.assignedRegister << ": web" << web.id << "\n";
        }
    }

    out.close();
}