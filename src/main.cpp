    #include <iostream>
    #include <string>
    #include <vector>

    // include our headers
    #include "Web.h"
    #include "Parser.h"
    #include "Allocator.h"

    // helper function to show the interactive menu
    void showMenu() {
        std::cout << "\n=============================================\n";
        std::cout << "  Compiler Register Allocation Tool\n";
        std::cout << "=============================================\n";
        std::cout << "1. Run Basic Allocation\n";
        std::cout << "2. Run Spilling Allocation\n";
        std::cout << "3. Run Splitting Allocation\n";
        std::cout << "4. Run Free Allocation\n";
        std::cout << "0. Exit\n";
        std::cout << "=============================================\n";
        std::cout << "Select an option: ";
    }

    int main(int argc, char* argv[]) {
        // T1.1: we need to support a batch mode executed via script[cite: 1]
        // the arguments must be: myProg -b ranges.txt registers.txt allocation.txt[cite: 1]
        bool isBatchMode = false;
        std::string rangesFile, registersFile, outputFile;

        if (argc >= 5 && std::string(argv[1]) == "-b") {
            isBatchMode = true;
            rangesFile = argv[2];
            registersFile = argv[3];
            outputFile = argv[4];
            std::cout << "Running in batch mode...\n";
        }

        if (isBatchMode) {
            // --- BATCH MODE EXECUTION ---
            try {
                // parse files
                std::vector<Web> webs = Parser::parseLiveRanges(rangesFile);
                Config config = Parser::parseConfig(registersFile);

                // setup allocator
                RegisterAllocator allocator(webs, config);

                // run the correct algorithm based on the config file
                if (config.algorithmType == "basic") {
                    allocator.runBasicAllocation();
                } else if (config.algorithmType == "spilling") {
                    allocator.runSpillingAllocation();
                } else if (config.algorithmType == "splitting") {
                    allocator.runSplittingAllocation();
                } else if (config.algorithmType == "free") {
                    allocator.runFreeAllocation();
                } else {
                    std::cerr << "Error: Unknown algorithm type in config file.\n";
                    return 1;
                }

                // finally -> generate output
                allocator.generateOutput(outputFile);
                std::cout << "Allocation complete. Results saved to " << outputFile << "\n";

            } catch (const std::exception& e) {
                // handle any parsing or logic errors gracefully
                std::cerr << "Error during batch execution: " << e.what() << "\n";
                return 1;
            }

        } else {
            // --- INTERACTIVE MENU EXECUTION ---
            // Teste Oficial 1 (Segundo a tabela: ranges1 + registers2)
            rangesFile = (argc > 1) ? argv[1] : "../ranges/ranges1.txt";
            registersFile = (argc > 2) ? argv[2] : "../registers/registers2.txt";
            // Guardar diretamente na pasta build para não haver problemas de pastas em falta
            outputFile = (argc > 3) ? argv[3] : "allocation_output.txt";

            int option = -1;
            while (option != 0) {
                showMenu();
                if (!(std::cin >> option)) {
                    // clear error state if user inputs a string instead of a number
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                    option = -1;
                }

                switch (option) {
                    case 1: {
                        std::cout << "Running basic allocation...\n";
                        // 1. Ler os ficheiros
                        std::vector<Web> webs = Parser::parseLiveRanges(rangesFile);
                        Config config = Parser::parseConfig(registersFile);
                        // 2. Criar o alocador e correr o algoritmo
                        RegisterAllocator allocator(webs, config);
                        allocator.runBasicAllocation();
                        // 3. Gerar o ficheiro de saída
                        allocator.generateOutput(outputFile);
                        std::cout << "Concluido! Ficheiro gerado em: " << outputFile << "\n";
                        break;
                    }
                    case 2: {
                        std::cout << "Running spilling allocation...\n";
                        std::vector<Web> webs = Parser::parseLiveRanges(rangesFile);
                        Config config = Parser::parseConfig(registersFile);
                        RegisterAllocator allocator(webs, config);
                        allocator.runSpillingAllocation();
                        allocator.generateOutput(outputFile);
                        std::cout << "Concluido! Ficheiro gerado em: " << outputFile << "\n";
                        break;
                    }
                    case 3:
                        std::cout << "Running splitting allocation...\n";
                        break;
                    case 4:
                        std::cout << "Running free allocation...\n";
                        break;
                    case 0:
                        std::cout << "Exiting...\n";
                        break;
                    default:
                        std::cout << "Invalid option. Please try again.\n";
                }
            }
        }

        return 0;
    }