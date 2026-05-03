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

    // Criamos uma lista de adjacências temporária para podermos ir "apagando"
    // os nós durante a Fase 1 sem estragarmos o grafo original.
    std::map<int, std::set<int>> adjList;
    for (auto* vertex : interferenceGraph.getVertexSet()) {
        int id = vertex->getInfo();
        adjList[id];
        for (auto* edge : vertex->getAdj()) {
            adjList[id].insert(edge->getDest()->getInfo());
        }
    }

    // ====================================================================
    // FASE 1: Simplificação (Drenar o grafo para a stack)
    // ====================================================================
    while (!adjList.empty()) {
        bool foundNodeToRemove = false;

        // Procuramos um nó que tenha um grau menor que o número de registos disponíveis
        for (auto it = adjList.begin(); it != adjList.end(); ++it) {
            int nodeId = it->first;
            int degree = it->second.size();

            if (degree < numRegisters) {
                // Se o grau é menor, é seguro metê-lo na stack
                s.push(nodeId);

                // Apagamos este nó da lista de contactos dos vizinhos dele
                for (int neighbor : it->second) {
                    adjList[neighbor].erase(nodeId);
                }

                // Removemos o nó do nosso grafo temporário
                adjList.erase(it);
                foundNodeToRemove = true;
                break; // Começamos a busca de novo desde o início
            }
        }

        // ====================================================================
        // O GRAFO FICOU PRESO (Todos os nós restantes têm grau >= numRegisters)
        // ====================================================================
        if (!foundNodeToRemove && !adjList.empty()) {

            // Em vez de desistir, vamos ver se ainda podemos deitar um nó para a memória (spill)
            if (spillsUsed < maxSpills) {

                // HEURÍSTICA DE INTELIGÊNCIA:
                // Vamos procurar o nó com o MAIOR GRAU (o que tem mais vizinhos).
                // Ao matarmos o vizinho mais problemático, aliviamos a pressão no resto do grafo!
                auto maxIt = adjList.begin();
                for (auto it = adjList.begin(); it != adjList.end(); ++it) {
                    if (it->second.size() > maxIt->second.size()) {
                        maxIt = it;
                    }
                }

                int nodeToSpill = maxIt->first;

                // Apagamos o nó problemático das listas dos vizinhos dele
                for (int neighbor : maxIt->second) {
                    adjList[neighbor].erase(nodeToSpill);
                }
                adjList.erase(maxIt); // Removemo-lo do grafo temporário

                // CRÍTICO: Marcamos logo este nó para ir para a memória (-1).
                // Como NÃO o colocamos na stack 's', a Fase 2 vai ignorá-lo completamente!
                auto itWeb = std::find_if(webs.begin(), webs.end(),
                                          [nodeToSpill](const Web& w){ return w.id == nodeToSpill; });
                if (itWeb != webs.end()) {
                    itWeb->assignedRegister = -1;
                }

                spillsUsed++;
                // O ciclo volta a rodar! Conseguimos desbloquear o grafo.

            } else {
                // Ficámos mesmo presos e já esgotámos os nossos "jokers" de spill.
                // O algoritmo falhou.
                return false;
            }
        }
    }

    // ====================================================================
    // FASE 2: Coloração (Atribuir registos aos nós que estão na stack)
    // ====================================================================

    // Um mapa rápido para conseguirmos encontrar as nossas Webs pelo ID
    std::map<int, Web*> webMap;
    for (auto& web : webs) {
        webMap[web.id] = &web;
    }

    // Tiramos os nós da stack um a um (o último a entrar é o primeiro a sair)
    while (!s.empty()) {
        int nodeId = s.top();
        s.pop();

        std::set<int> takenColors; // Cores que já estão ocupadas pelos vizinhos
        auto* vertex = interferenceGraph.findVertex(nodeId);

        // Vamos cuscar as cores dos vizinhos deste nó
        for (auto* edge : vertex->getAdj()) {
            int neighborId = edge->getDest()->getInfo();
            int neighborColor = webMap[neighborId]->assignedRegister;

            // Se o vizinho já tiver uma cor (não é -1), guardamos essa cor nas ocupadas
            if (neighborColor != -1) {
                takenColors.insert(neighborColor);
            }
        }

        // Atribuir a PRIMEIRA cor (registo) que esteja livre
        for (int color = 0; color < numRegisters; ++color) {
            if (takenColors.find(color) == takenColors.end()) {
                webMap[nodeId]->assignedRegister = color;
                break; // Cor atribuída, podemos passar ao próximo nó
            }
        }
    }

    return true; // Sucesso! O grafo foi colorido.
}

// builds the graph and tries to color it. if it fails, all webs go to memory (m).
void RegisterAllocator::runBasicAllocation() {
    buildInterferenceGraph();

    if (!colorGraph(config.maxRegisters,0)) {
        std::cerr << "Warning: Basic allocation failed. Not enough registers. Moving all to memory.\n";
        for (auto& web : webs) {
            web.assignedRegister = -1; // -1 means memory
        }
    }
}


// todo: if colorGraph() fails, pick 'k' webs to spill (via config.algoParameter),
// remove them from the graph and try coloring again.
void RegisterAllocator::runSpillingAllocation() {
    buildInterferenceGraph();

    // Na alocação com Spilling, deixamos o algoritmo deitar fora até 'K' webs
    // (este K vem do ficheiro de configuração, ex: algoritmo: spilling, 2)
    if (!colorGraph(config.maxRegisters, config.algoParameter)) {
        std::cerr << "Aviso: A alocacao falhou mesmo usando "
                  << config.algoParameter << " spills. Todas as variaveis vao para a memoria.\n";
        for (auto& web : webs) {
            web.assignedRegister = -1;
        }
    }
}

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