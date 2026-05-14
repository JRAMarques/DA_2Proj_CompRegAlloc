#include "io/OutputGenerator.h"
#include <iostream>
#include <fstream>
#include <filesystem>

void OutputGenerator::generate(const std::string& filename, const std::vector<Web>& webs, const Config& config) {
    std::filesystem::path p(filename);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
        if (ec) {
            std::cerr << "Warning: Could not create directory for " << filename << ". Error: " << ec.message() << "\n";
        }
    }

    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Error: Could not create output file: " << filename << "\n";
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
