#include "Parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <map>

// helper function to trim whitespace from strings
static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::vector<Web> Parser::parseLiveRanges(const std::string& filename) {
    std::vector<Web> webs;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open ranges file: " << filename << "\n";
        return webs;
    }

    std::string line;
    std::string currentVar = "";
    int webIdCounter = 0;

    // temporary structure to hold raw ranges before merging
    std::vector<Web> rawWebs;

    while (std::getline(file, line)) {
        line = trim(line);
        // ignore empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        // check if this line declares a variable (e.g., "sum: 7+,8,9,10-")
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string varPart = trim(line.substr(0, colonPos));
            if (!varPart.empty()) {
                currentVar = varPart; // update current variable context
            }
            line = line.substr(colonPos + 1); // keep only the numbers part
        }

        if (currentVar.empty()) continue; // skip if we haven't seen a variable name yet

        // create a temporary web for this specific line
        Web tempWeb(webIdCounter++, currentVar);
        std::stringstream ss(line);
        std::string token;

        // parse the comma-separated numbers
        while (std::getline(ss, token, ',')) {
            token = trim(token);
            if (token.empty()) continue;

            bool isDef = false;
            bool isUse = false;

            // check for '+' or '-' suffixes
            if (token.back() == '+') {
                isDef = true;
                token.pop_back();
            } else if (token.back() == '-') {
                isUse = true;
                token.pop_back();
            }

            try {
                int lineNum = std::stoi(token);
                tempWeb.activeLines.insert(lineNum);
                if (isDef) tempWeb.defLines.insert(lineNum);
                if (isUse) tempWeb.useLines.insert(lineNum);
            } catch (const std::invalid_argument& e) {
                std::cerr << "Error: '" << token << "' is not a valid number.\n";
            }
            catch (const std::out_of_range& e) {
                    std::cerr << "Error: Line number '" << token << "' is too large.\n";
            }
        }

        if (!tempWeb.activeLines.empty()) {
            rawWebs.push_back(tempWeb);
        }
    }

    file.close();

    // we need to group the raw webs by variable name first
    std::map<std::string, std::vector<Web>> websByVar;
    for (const auto& rw : rawWebs) {
        websByVar[rw.varName].push_back(rw);
    }

    int finalWebId = 0;

    // now we process each variable independently
    for (auto& pair : websByVar) {
        std::vector<Web>& varWebs = pair.second;

        // keep trying to merge until no more merges can happen
        bool mergedSomething = true;
        while (mergedSomething) {
            mergedSomething = false;

            for (size_t i = 0; i < varWebs.size(); ++i) {
                for (size_t j = i + 1; j < varWebs.size(); ++j) {

                    // if two ranges of the same variable overlap at any point,
                    // they are part of the same web!
                    if (varWebs[i].interferesWith(varWebs[j])) {

                        // merge all lines from web 'j' into web 'i'
                        varWebs[i].activeLines.insert(varWebs[j].activeLines.begin(), varWebs[j].activeLines.end());
                        varWebs[i].defLines.insert(varWebs[j].defLines.begin(), varWebs[j].defLines.end());
                        varWebs[i].useLines.insert(varWebs[j].useLines.begin(), varWebs[j].useLines.end());

                        // erase web 'j' since it's now swallowed by 'i'
                        varWebs.erase(varWebs.begin() + j);

                        mergedSomething = true;
                        break; // break the inner loop so we don't mess up the indices
                    }
                }
                if (mergedSomething) break; // break the outer loop and restart the check
            }
        }

        // after fully merging, assign the official IDs and save them to our final list
        for (auto& finalWeb : varWebs) {
            finalWeb.id = finalWebId++;
            webs.push_back(finalWeb);
        }
    }

    return webs;
}


Config Parser::parseConfig(const std::string& filename) {
    Config config;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open registers file: " << filename << "\n";
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = trim(line.substr(0, colonPos));
            std::string value = trim(line.substr(colonPos + 1));

            if (key == "registers") {
                try {
                    config.maxRegisters = std::stoi(value);
                } catch (...) {
                    std::cerr << "Warning: Invalid register count.\n";
                }
            } else if (key == "algorithm") {
                // handle parsing algorithms with parameters like "spilling, 2"
                size_t commaPos = value.find(',');
                if (commaPos != std::string::npos) {
                    config.algorithmType = trim(value.substr(0, commaPos));
                    try {
                        config.algoParameter = std::stoi(trim(value.substr(commaPos + 1)));
                    } catch (...) {}
                } else {
                    config.algorithmType = value;
                }
            }
        }
    }

    file.close();
    return config;
}