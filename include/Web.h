#ifndef WEB_H
#define WEB_H

#include <string>
#include <set>

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
};

#endif