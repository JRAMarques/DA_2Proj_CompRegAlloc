#ifndef OUTPUT_GENERATOR_H
#define OUTPUT_GENERATOR_H

#include <vector>
#include <string>
#include "Web.h"
#include "Parser.h"

/**
 * @brief Class responsible for exporting the register allocation results to a text file.
 */
class OutputGenerator {
public:
    /**
     * @brief Generates the output file in the required format.
     * @param filename Path to save the results.
     * @param webs The final allocated webs.
     * @param config The original configuration context.
     * @timecomplexity O(V * L)
     */
    static void generate(const std::string& filename, const std::vector<Web>& webs, const Config& config);
};

#endif
