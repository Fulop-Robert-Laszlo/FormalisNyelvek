#include "dfa.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <algorithm>

void DfaProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("check", "Check words with the DFA", cxxopts::value<std::string>())
        ("det", "Determinize the automaton");
}

bool DfaProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("check") > 0 || args.count("det") > 0;
}

int DfaProblem::run(const cxxopts::ParseResult &args) {
    if (args.count("check") > 0) {
        return run_check(args);
    }
    if (args.count("det") > 0) {
        return run_det(args);
    }
    return 1;
}

// ==================== 1. FELADAT: DFA SZIMULÁCIÓ ====================
int DfaProblem::run_check(const cxxopts::ParseResult &args) {
    std::string inputFilename = args["input"].as<std::string>();
    std::string outputFilename = args["output"].as<std::string>();
    std::string wordsToCheck = args["check"].as<std::string>();

    std::ifstream inputFile(inputFilename);
    if (!inputFile) {
        std::cerr << "Error opening input file: " << inputFilename << std::endl;
        return 1;
    }

    std::string line;
    std::getline(inputFile, line); // 1. sor: Állapotok
    std::getline(inputFile, line); // 2. sor: Ábécé

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

    // Átmenetek
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

        if (i + 1 < words.size()) {
            outputFile << "\n";
        }
    }

    outputFile.close();
    return 0;
}

// ==================== 2. FELADAT: DETERMINIZÁLÁS ====================
int DfaProblem::run_det(const cxxopts::ParseResult &args) {
    std::string inputFilename = args["input"].as<std::string>();
    std::string outputFilename = args["output"].as<std::string>();

    std::ifstream inputFile(inputFilename);
    if (!inputFile) {
        std::cerr << "Error opening input file: " << inputFilename << std::endl;
        return 1;
    }

    std::string line;

    // 1. sor: Állapotok beolvasása
    if (!std::getline(inputFile, line)) return 1;
    std::set<std::string> validStates;
    {
        std::istringstream ss(line);
        std::string s;
        while (ss >> s) validStates.insert(s);
    }
    if (validStates.empty()) return 1;

    // 2. sor: Bemeneti ábécé beolvasása (std::set automatikusan ábécé sorrendben tartja)
    if (!std::getline(inputFile, line)) return 1;
    std::set<std::string> alphabet;
    {
        std::istringstream ss(line);
        std::string sym;
        while (ss >> sym) alphabet.insert(sym);
    }
    if (alphabet.empty()) return 1;

    // 3. sor: Kezdőállapot beolvasása és ellenőrzése
    if (!std::getline(inputFile, line)) return 1;
    std::string startState;
    {
        std::istringstream ss(line);
        if (!(ss >> startState)) return 1;
        std::string extra;
        if (ss >> extra) return 1; // Csak egy kezdőállapot lehet
    }
    if (validStates.find(startState) == validStates.end()) {
        std::cerr << "Invalid start state!" << std::endl;
        return 1;
    }

    // 4. sor: Végállapotok beolvasása és ellenőrzése
    if (!std::getline(inputFile, line)) return 1;
    std::set<std::string> acceptStates;
    {
        std::istringstream ss(line);
        std::string s;
        while (ss >> s) {
            if (validStates.find(s) == validStates.end()) {
                std::cerr << "Invalid accept state: " << s << std::endl;
                return 1;
            }
            acceptStates.insert(s);
        }
    }

    // Következő sorok: Átmenetek beolvasása és hibakezelés
    std::map<std::pair<std::string, std::string>, std::set<std::string>> nfaTransitions;
    while (std::getline(inputFile, line)) {
        std::istringstream ss(line);
        std::string fromState, symbol, toState;
        if (!(ss >> fromState)) continue; // Üres sor átugrása
        if (!(ss >> symbol >> toState)) return 1; // Hiányos sor
        std::string extra;
        if (ss >> extra) return 1; // Túl sok elem a sorban

        // Ellenőrizzük, hogy az állapotok és a jel szerepelnek-e a fájl elején
        if (validStates.find(fromState) == validStates.end() ||
            validStates.find(toState) == validStates.end() ||
            alphabet.find(symbol) == alphabet.end()) {
            std::cerr << "Invalid transition: " << fromState << " " << symbol << " " << toState << std::endl;
            return 1;
        }

        nfaTransitions[{fromState, symbol}].insert(toState);
    }
    inputFile.close();

    // Determinizálás (Részhalmaz-konstrukció / BFS)
    std::map<std::set<std::string>, int> subsetToId;
    std::vector<std::set<std::string>> idToSubset;
    std::queue<std::set<std::string>> q;

    std::set<std::string> startSubset = {startState};
    subsetToId[startSubset] = 0;
    idToSubset.push_back(startSubset);
    q.push(startSubset);

    // Átmenetek tárolása: (forrás_id, szimbólum) -> cél_id
    std::map<std::pair<int, std::string>, int> dfaTransitions;

    while (!q.empty()) {
        std::set<std::string> currentSubset = q.front();
        q.pop();
        int currentId = subsetToId[currentSubset];

        for (const std::string& sym : alphabet) {
            std::set<std::string> nextSubset;
            for (const std::string& nfaState : currentSubset) {
                auto it = nfaTransitions.find({nfaState, sym});
                if (it != nfaTransitions.end()) {
                    nextSubset.insert(it->second.begin(), it->second.end());
                }
            }

            if (nextSubset.empty()) continue; // Ahol nincs átmenet, nem kerül felsorolásra

            if (subsetToId.find(nextSubset) == subsetToId.end()) {
                int newId = static_cast<int>(idToSubset.size());
                subsetToId[nextSubset] = newId;
                idToSubset.push_back(nextSubset);
                q.push(nextSubset);
            }

            dfaTransitions[{currentId, sym}] = subsetToId[nextSubset];
        }
    }

    // Állapotnevek és végállapotok előkészítése
    int numStates = static_cast<int>(idToSubset.size());
    std::vector<std::string> stateNames;
    std::vector<std::string> dfaAcceptStates;

    for (int i = 0; i < numStates; ++i) {
        std::string sName = "s" + std::to_string(i);
        stateNames.push_back(sName);

        bool isAccept = false;
        for (const std::string& nfaState : idToSubset[i]) {
            if (acceptStates.find(nfaState) != acceptStates.end()) {
                isAccept = true;
                break;
            }
        }
        if (isAccept) {
            dfaAcceptStates.push_back(sName);
        }
    }

    // Ábécé sorrend biztosítása (ha s10 vagy nagyobb indexű állapot is lenne)
    std::sort(stateNames.begin(), stateNames.end());
    std::sort(dfaAcceptStates.begin(), dfaAcceptStates.end());

    struct Edge {
        std::string from;
        std::string sym;
        std::string to;
        bool operator<(const Edge& other) const {
            if (from != other.from) return from < other.from;
            if (sym != other.sym) return sym < other.sym;
            return to < other.to;
        }
    };

    std::vector<Edge> edges;
    for (const auto& kv : dfaTransitions) {
        std::string fromName = "s" + std::to_string(kv.first.first);
        std::string sym = kv.first.second;
        std::string toName = "s" + std::to_string(kv.second);
        edges.push_back({fromName, sym, toName});
    }
    std::sort(edges.begin(), edges.end());

    // Eredmény kiírása a kimeneti fájlba
    std::ofstream outputFile(outputFilename);
    if (!outputFile) {
        std::cerr << "Error opening output file: " << outputFilename << std::endl;
        return 1;
    }

    // 1. sor: Állapotok
    for (size_t i = 0; i < stateNames.size(); ++i) {
        outputFile << stateNames[i] << (i + 1 < stateNames.size() ? " " : "");
    }
    outputFile << "\n";

    // 2. sor: Ábécé
    size_t idx = 0;
    for (const std::string& sym : alphabet) {
        outputFile << sym << (idx + 1 < alphabet.size() ? " " : "");
        idx++;
    }
    outputFile << "\n";

    // 3. sor: Kezdőállapot
    outputFile << "s0\n";

    // 4. sor: Végállapotok
    for (size_t i = 0; i < dfaAcceptStates.size(); ++i) {
        outputFile << dfaAcceptStates[i] << (i + 1 < dfaAcceptStates.size() ? " " : "");
    }
    outputFile << "\n";

    // Következő sorok: Átmenetek
    for (size_t i = 0; i < edges.size(); ++i) {
        outputFile << edges[i].from << " " << edges[i].sym << " " << edges[i].to;
        if (i + 1 < edges.size()) {
            outputFile << "\n";
        }
    }

    outputFile.close();
    return 0;
}