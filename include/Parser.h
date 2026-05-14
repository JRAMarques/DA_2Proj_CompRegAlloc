#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include "Web.h"

/**
 * @brief Simple struct to hold the configuration parsed from the registers.txt file.
 */
struct Config {
    int maxRegisters = 0;
    std::string algorithmType = "basic";
    int algoParameter = 0; // useful for spilling/splitting where we get a 'k' value (e.g., "spilling, 2")
};

/**
 * @brief Class responsible for reading the input datasets and converting them into usable data structures.
 *
 * @details This is mostly a static utility class since we just need it to swallow files
 * and spit out our Web objects and Config settings.
 */
class Parser {
public:
    /**
     * @brief Reads the live ranges file and merges overlapping ranges into distinct Webs.
     *
     * @details This function will do the heavy lifting of reading the lines, looking for '+' and '-',
     * and merging continuous usages of the same variable into a single Web object.
     *
     * @param filename The path to the ranges.txt file.
     * @return std::vector<Web> A list of fully built Web objects ready for the interference graph.
     * 
     * @timecomplexity O(N^2 * L) where N is the number of raw live ranges parsed and L is the average number of active lines per range.
     */
    static std::vector<Web> parseLiveRanges(const std::string& filename);

    /**
     * @brief Reads the register configuration file.
     *
     * @param filename The path to the registers.txt file.
     * @return Config A struct containing the max registers and the algorithm to run.
     * 
     * @timecomplexity O(L) where L is the number of lines in the configuration file.
     */
    static Config parseConfig(const std::string& filename);
};

#endif