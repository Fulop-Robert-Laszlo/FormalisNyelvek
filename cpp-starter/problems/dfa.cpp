#include "dfa.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>

// A --check kapcsoló hozzáadása a parserhez
void DfaProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("check", "Check words with the DFA", cxxopts::value<std::string>());
}

// Akkor választjuk ezt a feladatot, ha megadták a --check kapcsolót
bool DfaProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("check") > 0;
}

// A DFA szimuláció futtatása
int DfaProblem::run(const cxxopts::ParseResult &args) {
    std::string inputFilename = args["input"].as<std::string>();
    std::string outputFilename = args["output"].as<std::string>();
    std::string wordsToCheck = args["check"].as<std::string>();

    std::ifstream inputFile(inputFilename);
    if (!inputFile) {
        std::cerr << "Error opening input file: " << inputFilename << std::endl;
        return 1;
    }

    std::string line;

    // 1. sor: Állapotok
    std::getline(inputFile, line);

    // 2. sor: Bemeneti ábécé
    std::getline(inputFile, line);

    // 3. sor: Kezdőállapot
    std::getline(inputFile, line);
    std::string startState;
    std::istringstream ssStart(line);
    ssStart >> startState;

    // 4. sor: Végállapotok
    std::getline(inputFile, line);
    std::set<std::string> acceptStates;
    std::istringstream ssAccept(line);
    std::string state;
    while (ssAccept >> state) {
        acceptStates.insert(state);
    }

    // További sorok: Átmenetek (állapot1 szimbólum állapot2)
    std::map<std::pair<std::string, char>, std::string> transitions;
    while (std::getline(inputFile, line)) {
        if (line.empty()) continue;
        std::istringstream ssTrans(line);
        std::string fromState, toState;
        char symbol;
        if (ssTrans >> fromState >> symbol >> toState) {
            transitions[{fromState, symbol}] = toState;
        }
    }
    inputFile.close();

    // A --check után megadott, vesszővel elválasztott szavak darabolása
    std::vector<std::string> words;
    std::stringstream ssWords(wordsToCheck);
    std::string word;
    while (std::getline(ssWords, word, ',')) {
        words.push_back(word);
    }

    std::ofstream outputFile(outputFilename);
    if (!outputFile) {
        std::cerr << "Error opening output file: " << outputFilename << std::endl;
        return 1;
    }

    // Szavak kiértékelése egyesével
    for (size_t i = 0; i < words.size(); ++i) {
        std::string currentState = startState;
        bool valid = true;

        for (char c : words[i]) {
            if (c == '\r' || c == ' ') continue;
            auto it = transitions.find({currentState, c});
            if (it != transitions.end()) {
                currentState = it->second;
            } else {
                valid = false;
                break;
            }
        }

        if (valid && acceptStates.find(currentState) != acceptStates.end()) {
            outputFile << "IGEN";
        } else {
            outputFile << "NEM";
        }

        // Ha több szó van, új sor választja el őket
        if (i + 1 < words.size()) {
            outputFile << "\n";
        }
    }

    outputFile.close();
    return 0;
}