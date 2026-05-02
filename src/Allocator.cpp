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
bool RegisterAllocator::colorGraph(int numRegisters) {
    std::stack<int> s;

    // temporary adjacency list so we don't touch the actual graph
    std::map<int, std::set<int>> adjList;
    for (auto* vertex : interferenceGraph.getVertexSet()) {
        int id = vertex->getInfo();
        for (auto* edge : vertex->getAdj()) {
            adjList[id].insert(edge->getDest()->getInfo());
        }
    }

    // phase 1: drain the graph onto the stack
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

        // stuck — basic allocation fails here
        if (!foundNodeToRemove && !adjList.empty()) {
            return false;
        }
    }

    // phase 2: color as we pop
    std::map<int, Web*> webMap;
    for (auto& web : webs) {
        webMap[web.id] = &web;
    }

    while (!s.empty()) {
        int nodeId = s.top();
        s.pop();

        // collect colors already taken by neighbors
        std::set<int> takenColors;
        auto* vertex = interferenceGraph.findVertex(nodeId);

        for (auto* edge : vertex->getAdj()) {
            int neighborId = edge->getDest()->getInfo();
            int neighborColor = webMap[neighborId]->assignedRegister;
            if (neighborColor != -1) {
                takenColors.insert(neighborColor);
            }
        }

        // assign the first free color
        for (int color = 0; color < numRegisters; ++color) {
            if (takenColors.find(color) == takenColors.end()) {
                webMap[nodeId]->assignedRegister = color;
                break;
            }
        }
    }

    return true;
}

// builds the graph and tries to color it. if it fails, all webs go to memory (m).
void RegisterAllocator::runBasicAllocation() {
    buildInterferenceGraph();

    if (!colorGraph(config.maxRegisters)) {
        std::cerr << "Warning: Basic allocation failed. Not enough registers. Moving all to memory.\n";
        for (auto& web : webs) {
            web.assignedRegister = -1; // -1 means memory
        }
    }
}


// todo: if colorGraph() fails, pick 'k' webs to spill (via config.algoParameter),
// remove them from the graph and try coloring again.
void RegisterAllocator::runSpillingAllocation() {}

void RegisterAllocator::runSplittingAllocation() {}

void RegisterAllocator::runFreeAllocation() {}


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