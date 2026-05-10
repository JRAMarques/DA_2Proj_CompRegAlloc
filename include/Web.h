#ifndef WEB_H
#define WEB_H

#include <string>
#include <set>
#include <utility>

/**
 * @brief Represents a single web (a union of overlapping live ranges for a specific variable).
 *
 * We use this structure to track a variable's lifetime throughout the code.
 * Ultimately, each Web will become a node in our interference graph!
 */
struct Web {
    /**
     * @brief Unique ID so we can easily map this web to a node in our interference graph.
     */
    int id;

    /**
     * @brief The actual name of the variable from the source file (e.g., "sum", "i").
     */
    std::string varName;

    /**
     * @brief Stores all the lines where this web is currently alive.
     *
     * @details Using a std::set is a lifesaver here because it automatically keeps
     * the line numbers sorted in ascending order and removes any duplicates when we merge ranges.
     */
    std::set<int> activeLines;

    /**
     * @brief Remembers the exact points where the variable is defined (+).
     * This might be super useful when deciding which web to split or spill later on.
     */
    std::set<int> defLines;

    /**
     * @brief Remembers the exact points where the variable is used (-).
     */
    std::set<int> useLines;

    /**
     * @brief The physical register assigned by our graph coloring algorithm.
     *
     * @details According to the spec, we start with no assignment (-1).
     * If it remains -1 after the algorithm runs, it means we must spill it to memory ('M').
     */
    int assignedRegister = -1;

    /**
     * @brief Quick constructor to keep our instantiation clean.
     *
     * @param assignedId The unique identifier we want to give to this web.
     * @param name The original variable name parsed from the file.
     */
    Web(int assignedId, const std::string& name) : id(assignedId), varName(name) {}

    /**
     * @brief Handy helper function to check if this web intersects with another one.
     *
     * @details This will be the core logic to build the edges of our interference graph.
     * If we find even one common active line between the two webs, boom, they interfere.
     *
     * @param other The other web we are checking against.
     * @return true if the webs share at least one active line, false otherwise.
     *
     * @note Time Complexity: O(N log M), where N is the number of active lines in this web,
     * and M is the number of active lines in the other web (due to the std::set::find lookup).
     */
    bool interferesWith(const Web& other) const {
        for (int line : activeLines) {
            if (other.activeLines.find(line) != other.activeLines.end()) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Operator overloading so the Graph template can compare Webs.
     */
    bool operator==(const Web& other) const {
        return this->id == other.id;
    }

    /**
     * @brief Analisa os live ranges para detetar o maior intervalo de inatividade.
     * @return pair<int, int> onde o first é o tamanho do maior intervalo e o second é a linha de corte.
     */
    std::pair<int, int> findBiggestHole() const {
        if (activeLines.size() < 2) {
            return {0, -1}; //não dá para dividir uma web com menos de 2 linhas
        }

        int maxGap = 0;
        int splitPoint = -1;
        int previousLine = -1;

        //o set já garante que as linhas estão ordenadas cronologicamente
        for (int line : activeLines) {
            if (previousLine != -1) {
                int gap = line - previousLine;

                //um gap > 1 significa que existe pelo menos uma linha de inatividade
                if (gap > maxGap && gap > 1) {
                    maxGap = gap;
                    splitPoint = line;
                }
            }
            previousLine = line;
        }
        return {maxGap, splitPoint};
    }

    /**
     * @brief Divide físicamente a Web em duas instâncias independentes com base num ponto de corte.
     * @param newId1 O ID a atribuir à primeira fração.
     * @param newId2 O ID a atribuir à segunda fração.
     * @param splitPoint Linha que delimita a fronteira da divisão.
     * @return Um std::pair contendo as duas novas Webs instanciadas(_A e _B).
     */
    std::pair<Web, Web> split(int newId1, int newId2, int splitPoint) const {
        //acrescentar os sufixos _A e _B para distinguir as variáveis fracionadas no output
        Web part1(newId1, this->varName + "_A");
        Web part2(newId2, this->varName + "_B");

        //distribuição das linhas ativas
        for (int line : this->activeLines) {
            if (line < splitPoint) part1.activeLines.insert(line);
            else part2.activeLines.insert(line);
        }

        //distribuição das definições (+)
        for (int line : this->defLines) {
            if (line < splitPoint) part1.defLines.insert(line);
            else part2.defLines.insert(line);
        }

        //distribuição das utilizações (-)
        for (int line : this->useLines) {
            if (line < splitPoint) part1.useLines.insert(line);
            else part2.useLines.insert(line);
        }

        return {part1, part2};
    }
};

#endif