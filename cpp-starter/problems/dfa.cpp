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
        ("det", "Determinize the automaton")
        ("mini", "Minimize the automaton");
}

bool DfaProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("check") > 0 || args.count("det") > 0 || args.count("mini") > 0;
}

int DfaProblem::run(const cxxopts::ParseResult &args) {
    if (args.count("check") > 0) {
        return run_check(args);
    }
    if (args.count("det") > 0) {
        return run_det(args);
    }
    if (args.count("mini") > 0) {
        return run_mini(args);
    }
    return 1;
}

// ==================== 1. FELADAT: DFA SZIMULÁCIÓ ====================
int DfaProblem::run_check(const cxxopts::ParseResult &args) {
    std::string inputFilename = args["input"].as<std::string>();
    std::string outputFilename = args["output"].as<std::string>();
    std::string wordsToCheck = args["check"].as<std::string>();

    std::ifstream inputFile(inputFilename);
    if (!inputFile) return 1;

    std::string line;
    std::getline(inputFile, line);
    std::getline(inputFile, line);

    std::getline(inputFile, line);
    std::string startState;
    std::istringstream ssStart(line);
    ssStart >> startState;

    std::getline(inputFile, line);
    std::set<std::string> acceptStates;
    std::istringstream ssAccept(line);
    std::string state;
    while (ssAccept >> state) acceptStates.insert(state);

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
    while (std::getline(ssWords, word, ',')) words.push_back(word);

    std::ofstream outputFile(outputFilename);
    if (!outputFile) return 1;

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
        if (i + 1 < words.size()) outputFile << "\n";
    }
    outputFile.close();
    return 0;
}

// ==================== 2. FELADAT: DETERMINIZÁLÁS ====================
int DfaProblem::run_det(const cxxopts::ParseResult &args) {
    std::string inputFilename = args["input"].as<std::string>();
    std::string outputFilename = args["output"].as<std::string>();

    std::ifstream inputFile(inputFilename);
    if (!inputFile) return 1;

    std::string line;
    if (!std::getline(inputFile, line)) return 1;
    std::set<std::string> validStates;
    { std::istringstream ss(line); std::string s; while (ss >> s) validStates.insert(s); }
    if (validStates.empty()) return 1;

    if (!std::getline(inputFile, line)) return 1;
    std::set<std::string> alphabet;
    { std::istringstream ss(line); std::string sym; while (ss >> sym) alphabet.insert(sym); }
    if (alphabet.empty()) return 1;

    if (!std::getline(inputFile, line)) return 1;
    std::string startState;
    {
        std::istringstream ss(line);
        if (!(ss >> startState)) return 1;
        std::string extra;
        if (ss >> extra) return 1;
    }
    if (validStates.find(startState) == validStates.end()) return 1;

    if (!std::getline(inputFile, line)) return 1;
    std::set<std::string> acceptStates;
    {
        std::istringstream ss(line); std::string s;
        while (ss >> s) {
            if (validStates.find(s) == validStates.end()) return 1;
            acceptStates.insert(s);
        }
    }

    std::map<std::pair<std::string, std::string>, std::set<std::string>> nfaTransitions;
    while (std::getline(inputFile, line)) {
        std::istringstream ss(line);
        std::string fromState, symbol, toState;
        if (!(ss >> fromState)) continue;
        if (!(ss >> symbol >> toState)) return 1;
        std::string extra; if (ss >> extra) return 1;

        if (validStates.find(fromState) == validStates.end() ||
            validStates.find(toState) == validStates.end() ||
            alphabet.find(symbol) == alphabet.end()) return 1;
        nfaTransitions[{fromState, symbol}].insert(toState);
    }
    inputFile.close();

    std::map<std::set<std::string>, int> subsetToId;
    std::vector<std::set<std::string>> idToSubset;
    std::queue<std::set<std::string>> q;

    std::set<std::string> startSubset = {startState};
    subsetToId[startSubset] = 0;
    idToSubset.push_back(startSubset);
    q.push(startSubset);

    std::map<std::pair<int, std::string>, int> dfaTransitions;
    while (!q.empty()) {
        std::set<std::string> currentSubset = q.front(); q.pop();
        int currentId = subsetToId[currentSubset];

        for (const std::string& sym : alphabet) {
            std::set<std::string> nextSubset;
            for (const std::string& nfaState : currentSubset) {
                auto it = nfaTransitions.find({nfaState, sym});
                if (it != nfaTransitions.end()) nextSubset.insert(it->second.begin(), it->second.end());
            }
            if (nextSubset.empty()) continue;

            if (subsetToId.find(nextSubset) == subsetToId.end()) {
                int newId = static_cast<int>(idToSubset.size());
                subsetToId[nextSubset] = newId;
                idToSubset.push_back(nextSubset);
                q.push(nextSubset);
            }
            dfaTransitions[{currentId, sym}] = subsetToId[nextSubset];
        }
    }

    int numStates = static_cast<int>(idToSubset.size());
    std::vector<std::string> stateNames;
    std::vector<std::string> dfaAcceptStates;

    for (int i = 0; i < numStates; ++i) {
        std::string sName = "s" + std::to_string(i);
        stateNames.push_back(sName);
        bool isAccept = false;
        for (const std::string& nfaState : idToSubset[i]) {
            if (acceptStates.find(nfaState) != acceptStates.end()) { isAccept = true; break; }
        }
        if (isAccept) dfaAcceptStates.push_back(sName);
    }
    std::sort(stateNames.begin(), stateNames.end());
    std::sort(dfaAcceptStates.begin(), dfaAcceptStates.end());

    struct Edge {
        std::string from; std::string sym; std::string to;
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

    std::ofstream outputFile(outputFilename);
    if (!outputFile) return 1;

    for (size_t i = 0; i < stateNames.size(); ++i) {
        outputFile << stateNames[i] << (i + 1 < stateNames.size() ? " " : "");
    }
    outputFile << "\n";
    size_t idx = 0;
    for (const std::string& sym : alphabet) {
        outputFile << sym << (idx + 1 < alphabet.size() ? " " : ""); idx++;
    }
    outputFile << "\n";
    outputFile << "s0\n";
    for (size_t i = 0; i < dfaAcceptStates.size(); ++i) {
        outputFile << dfaAcceptStates[i] << (i + 1 < dfaAcceptStates.size() ? " " : "");
    }
    outputFile << "\n";
    for (size_t i = 0; i < edges.size(); ++i) {
        outputFile << edges[i].from << " " << edges[i].sym << " " << edges[i].to;
        if (i + 1 < edges.size()) outputFile << "\n";
    }
    outputFile.close();
    return 0;
}

// ==================== 3. FELADAT: MINIMALIZÁLÁS ====================
int DfaProblem::run_mini(const cxxopts::ParseResult &args) {
    std::string inputFilename = args["input"].as<std::string>();
    std::string outputFilename = args["output"].as<std::string>();

    std::ifstream inputFile(inputFilename);
    if (!inputFile) return 1;

    std::string line;

    // 1. Állapotok beolvasása (validációs halmazzal)
    if (!std::getline(inputFile, line)) return 1;
    std::vector<std::string> states;
    std::set<std::string> validStates;
    {
        std::istringstream ss(line);
        std::string s;
        while (ss >> s) {
            states.push_back(s);
            validStates.insert(s);
        }
    }

    // 2. Ábécé beolvasása
    if (!std::getline(inputFile, line)) return 1;
    std::vector<std::string> alphabet;
    std::set<std::string> validAlphabet;
    {
        std::istringstream ss(line);
        std::string sym;
        while (ss >> sym) {
            alphabet.push_back(sym);
            validAlphabet.insert(sym);
        }
    }

    // 3. Kezdőállapot
    if (!std::getline(inputFile, line)) return 1;
    std::string startState;
    {
        std::istringstream ss(line);
        if (!(ss >> startState)) return 1;
    }
    if (validStates.find(startState) == validStates.end()) return 1;

    // 4. Végállapotok
    if (!std::getline(inputFile, line)) return 1;
    std::set<std::string> acceptStates;
    {
        std::istringstream ss(line);
        std::string s;
        while (ss >> s) {
            if (validStates.find(s) == validStates.end()) return 1;
            acceptStates.insert(s);
        }
    }

    // Átmenetek beolvasása hibakezeléssel
    std::map<std::pair<std::string, std::string>, std::string> transitions;
    while (std::getline(inputFile, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::string fromState, symbol, toState;
        if (!(ss >> fromState >> symbol >> toState)) return 1;

        if (validStates.find(fromState) == validStates.end() ||
            validStates.find(toState) == validStates.end() ||
            validAlphabet.find(symbol) == validAlphabet.end()) return 1;

        transitions[{fromState, symbol}] = toState;
    }
    inputFile.close();

    // ================== MYHILL-NERODE TÁBLÁZATOS ALGORITMUS ==================
    // Megfelel az elküldött képen látható táblázatos/lépcsős levezetésnek
    int n = states.size();
    
    // table[i][j] igaz (true) lesz, ha states[i] és states[j] megkülönböztethető ("x" került a táblázatba)
    std::vector<std::vector<bool>> table(n, std::vector<bool>(n, false));

    // 1. lépés (Alapeset / egyenlőségvizsgálat): 
    // Ha egyik elfogadó, a másik nem, akkor egyből különböznek.
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            bool iAccepts = acceptStates.find(states[i]) != acceptStates.end();
            bool jAccepts = acceptStates.find(states[j]) != acceptStates.end();
            if (iAccepts != jAccepts) {
                table[i][j] = true;
                table[j][i] = true;
            }
        }
    }

    // 2. lépés (Folyamatos kitöltés / táblázat frissítése):
    // Addig iterálunk, amíg a táblázatban nem tudunk új "x"-et beírni (nincs változás).
    bool changed;
    do {
        changed = false;
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                if (!table[i][j]) { // Ha MÉG NEM különböznek
                    // Vizsgáljuk meg minden betűre, hogy hová visz az átmenet
                    for (const std::string& sym : alphabet) {
                        auto itI = transitions.find({states[i], sym});
                        auto itJ = transitions.find({states[j], sym});
                        
                        // Ha mindkettőnek van átmenete, megnézzük, hová mutat
                        if (itI != transitions.end() && itJ != transitions.end()) {
                            std::string nextI = itI->second;
                            std::string nextJ = itJ->second;
                            
                            // Megkeressük a következő állapotok indexeit a states vektorban
                            int idxI = std::distance(states.begin(), std::find(states.begin(), states.end(), nextI));
                            int idxJ = std::distance(states.begin(), std::find(states.begin(), states.end(), nextJ));
                            
                            // Ha a célállapotok MÁR meg vannak különböztetve, akkor ezek az állapotok is különböznek
                            if (table[idxI][idxJ]) {
                                table[i][j] = true;
                                table[j][i] = true;
                                changed = true;
                                break; // Egy elég a megkülönböztetéshez
                            }
                        }
                    }
                }
            }
        }
    } while (changed);

    // ================== ÖSSZEVONÁS ÉS ÚJRANEVEZÉS ==================
    // Ha table[i][j] maradt false, akkor states[i] és states[j] ekvivalens, tehát összevonhatók.
    // A feladat szerint a legkisebb indexű (q sorszám) állapot nevét kapja az összevont csoport.
    
    // create mapping from original state to its representative
    std::map<std::string, std::string> representative;
    for (int i = 0; i < n; ++i) {
        representative[states[i]] = states[i]; // Kezdetben önmaga a képviselő
    }

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (!table[i][j]) {
                // states[i] és states[j] egyenértékűek.
                std::string rootI = representative[states[i]];
                std::string rootJ = representative[states[j]];
                
                // Mivel a states tömb a bemenet alapján (q0, q1...) van feltöltve,
                // az elején lévők a kisebb sorszámúak, így ha kisebbet találunk, azt tesszük meg gyökérnek.
                if (rootI != rootJ) {
                    std::string newRoot = (rootI < rootJ) ? rootI : rootJ;
                    // Mindenkinek, aki rootI vagy rootJ volt, beállítjuk az újat
                    for (auto& pair : representative) {
                        if (pair.second == rootI || pair.second == rootJ) {
                            pair.second = newRoot;
                        }
                    }
                }
            }
        }
    }

    // Új állapotok összegyűjtése és "s"-formátumra konvertálása
    std::set<std::string> minimalStates;
    for (const auto& s : states) {
        minimalStates.insert(representative[s]);
    }

    // Létrehozunk egy q -> s map-et, hogy pl. "q1" -> "s1" legyen.
    // A feladat szerint ha a képviselő "q1", a kimeneti név "s1" kell hogy legyen.
    std::map<std::string, std::string> newNames;
    for (const std::string& state : minimalStates) {
        std::string numPart = state.substr(1); // Levágja a 'q'-t
        newNames[state] = "s" + numPart;
    }

    std::vector<std::string> outStates;
    for (const std::string& state : minimalStates) {
        outStates.push_back(newNames[state]);
    }
    std::sort(outStates.begin(), outStates.end()); // ábécé sorrend miatt

    // Kezdőállapot meghatározása (az eredeti kezdőállapot képviselője, 's'-es formában)
    std::string outStart = newNames[representative[startState]];

    // Végállapotok meghatározása (ha a képviselő egy eredeti elfogadó állapot, az egész csoport elfogadó)
    std::set<std::string> outAcceptSet;
    for (const std::string& accept : acceptStates) {
        outAcceptSet.insert(newNames[representative[accept]]);
    }
    std::vector<std::string> outAccepts(outAcceptSet.begin(), outAcceptSet.end());
    std::sort(outAccepts.begin(), outAccepts.end());

    // Átmenetek átszámítása az összevont állapotokra
    struct Edge {
        std::string from; std::string sym; std::string to;
        bool operator<(const Edge& other) const {
            if (from != other.from) return from < other.from;
            if (sym != other.sym) return sym < other.sym;
            return to < other.to;
        }
    };
    std::set<Edge> minimalEdgesSet; // A set megakadályozza a duplikációt
    for (const auto& trans : transitions) {
        std::string fromOrig = trans.first.first;
        std::string sym = trans.first.second;
        std::string toOrig = trans.second;

        std::string fromNew = newNames[representative[fromOrig]];
        std::string toNew = newNames[representative[toOrig]];
        
        minimalEdgesSet.insert({fromNew, sym, toNew});
    }

    std::vector<Edge> outEdges(minimalEdgesSet.begin(), minimalEdgesSet.end());
    // std::set alapból rendezi a fenti < operátor szerint, de biztos ami biztos:
    std::sort(outEdges.begin(), outEdges.end());

    // ================== EREDMÉNY KIÍRÁSA A MEGADOTT FORMÁTUMBA ==================
    std::ofstream outputFile(outputFilename);
    if (!outputFile) return 1;

    // 1. sor: Állapotok (s formában)
    for (size_t i = 0; i < outStates.size(); ++i) {
        outputFile << outStates[i] << (i + 1 < outStates.size() ? " " : "");
    }
    outputFile << "\n";

    // 2. sor: Ábécé
    // Itt a validAlphabet halmazt használjuk a kiíráshoz, hogy ábécé sorrendben maradjon
    size_t idx = 0;
    for (const std::string& sym : validAlphabet) {
        outputFile << sym << (idx + 1 < validAlphabet.size() ? " " : "");
        idx++;
    }
    outputFile << "\n";

    // 3. sor: Kezdőállapot
    outputFile << outStart << "\n";

    // 4. sor: Végállapotok
    for (size_t i = 0; i < outAccepts.size(); ++i) {
        outputFile << outAccepts[i] << (i + 1 < outAccepts.size() ? " " : "");
    }
    outputFile << "\n";

    // Átmenetek sorban
    for (size_t i = 0; i < outEdges.size(); ++i) {
        outputFile << outEdges[i].from << " " << outEdges[i].sym << " " << outEdges[i].to;
        if (i + 1 < outEdges.size()) outputFile << "\n";
    }

    outputFile.close();
    return 0;
}