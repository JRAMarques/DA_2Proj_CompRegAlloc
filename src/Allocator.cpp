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

    // Cópia temporária da lista de adjacências para manipulação durante a Fase 1
    std::map<int, std::set<int>> adjList;
    for (auto* vertex : interferenceGraph.getVertexSet()) {
        int id = vertex->getInfo();
        adjList[id];
        for (auto* edge : vertex->getAdj()) {
            adjList[id].insert(edge->getDest()->getInfo());
        }
    }

    //fase 1: Drenagem do grafo para a stack
    while (!adjList.empty()) {
        bool foundNodeToRemove = false;

        for (auto it = adjList.begin(); it != adjList.end(); ++it) {
            int nodeId = it->first;
            int degree = it->second.size();

            // Nós com grau inferior aos registos disponíveis vão para a stack
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

        // Deadlock: Todos os nós restantes têm grau >= numRegisters
        if (!foundNodeToRemove && !adjList.empty()) {
            if (spillsUsed < maxSpills) {
                // Heurística de Spilling: Escolher o nó com maior grau para atirar para a memória
                auto maxIt = adjList.begin();
                for (auto it = adjList.begin(); it != adjList.end(); ++it) {
                    if (it->second.size() > maxIt->second.size()) {
                        maxIt = it;
                    }
                }

                int nodeToSpill = maxIt->first;

                //apagamo-se o nó problemático das listas dos vizinhos dele
                for (int neighbor : maxIt->second) {
                    adjList[neighbor].erase(nodeToSpill);
                }
                adjList.erase(maxIt); //remove-lo do grafo temporário

                //o nó removido não entra na stack e é marcado como memória (-1)
                auto itWeb = std::find_if(webs.begin(), webs.end(),
                                          [nodeToSpill](const Web& w){ return w.id == nodeToSpill; });
                if (itWeb != webs.end()) {
                    itWeb->assignedRegister = -1;
                }
                spillsUsed++;
            } else {
                //falha na coloração após esgotar tentativas de spill
                return false;
            }
        }
    }

    // Fase 2: Atribuição de cores (registos)
    std::map<int, Web*> webMap;
    for (auto& web : webs) {
        webMap[web.id] = &web;
    }

    //tira-se os nós da stack um a um (o último a entrar é o primeiro a sair)
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

void RegisterAllocator::runSplittingAllocation() {
    int maxSplits = config.algoParameter;
    int splitsPerformed = 0;
    bool success = false;

    // 1. Tentar uma coloração inicial sem qualquer divisão
    buildInterferenceGraph();
    success = colorGraph(config.maxRegisters, 0);

    // 2. Se falhar, iniciar o processo iterativo de Splitting
    while (!success && splitsPerformed < maxSplits) {
        int bestWebIndex = -1;
        int maxHoleSize = 0;
        int bestSplitPoint = -1;

        // Procurar a Web com o maior "buraco" (intervalo de inatividade)
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

        // Condição de paragem: interrupção se não existirem buracos elegíveis para divisão
        if (bestWebIndex == -1 || maxHoleSize <= 1) {
            break;
        }

        // Extrair os dados necessários para a operação de divisão
        Web targetWeb = webs[bestWebIndex];

        // Determinar os próximos IDs disponíveis para as novas frações, garantindo a unicidade no grafo
        int maxId = 0;
        for (const auto& w : webs) {
            if (w.id > maxId) {
                maxId = w.id;
            }
        }

        // Executar a divisão física da variável
        auto newWebs = targetWeb.split(maxId + 1, maxId + 2, bestSplitPoint);

        // Substituir a Web original pelas duas frações no vetor de processamento
        webs.erase(webs.begin() + bestWebIndex);
        webs.push_back(newWebs.first);
        webs.push_back(newWebs.second);

        splitsPerformed++;

        // Limpar os vértices do grafo de interferência antigo antes de iniciar a reconstrução
        std::vector<int> nodesToRemove;
        for (auto* vertex : interferenceGraph.getVertexSet()) {
            nodesToRemove.push_back(vertex->getInfo());
        }
        for (int id : nodesToRemove) {
            interferenceGraph.removeVertex(id);
        }

        // Reconstruir o grafo com a nova topologia fracionada e testar a coloração
        buildInterferenceGraph();
        success = colorGraph(config.maxRegisters, 0);
    }

    // 3. Procedimento de Recurso: Se a coloração permanecer impossível após esgotar o limite K,
    // as variáveis não alocadas transitam, por inerência, para a memória.
    if (!success) {
        std::cerr << "Aviso: A alocacao com Splitting esgotou as " << splitsPerformed
                  << " divisoes e ainda carece de registos. As variaveis sobrantes transitam para a memoria.\n";
        for (auto& web : webs) {
            if (web.assignedRegister == -1) {
                web.assignedRegister = -1; // Consolida a marcação 'M' no output
            }
        }
    }
}
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