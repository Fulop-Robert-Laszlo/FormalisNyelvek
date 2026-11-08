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
#include <regex>

void DfaProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("check", "Check words with the DFA", cxxopts::value<std::string>())
        ("det", "Determinize the automaton")
        ("mini", "Minimize the automaton")
        ("rgx", "Regex parsing", cxxopts::value<int>());
}

bool DfaProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("check") > 0 || args.count("det") > 0 || args.count("mini") > 0 || args.count("rgx") > 0;
}

int DfaProblem::run(const cxxopts::ParseResult &args) {
    if (args.count("check") > 0) return run_check(args);
    if (args.count("det") > 0) return run_det(args);
    if (args.count("mini") > 0) return run_mini(args);
    if (args.count("rgx") > 0) return run_rgx(args);
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
    if (!std::getline(inputFile, line)) return 1;
    std::vector<std::string> states;
    std::set<std::string> validStates;
    {
        std::istringstream ss(line);
        std::string s;
        while (ss >> s) { states.push_back(s); validStates.insert(s); }
    }

    if (!std::getline(inputFile, line)) return 1;
    std::vector<std::string> alphabet;
    std::set<std::string> validAlphabet;
    {
        std::istringstream ss(line);
        std::string sym;
        while (ss >> sym) { alphabet.push_back(sym); validAlphabet.insert(sym); }
    }

    if (!std::getline(inputFile, line)) return 1;
    std::string startState;
    { std::istringstream ss(line); if (!(ss >> startState)) return 1; }
    if (validStates.find(startState) == validStates.end()) return 1;

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

    int n = states.size();
    std::vector<std::vector<bool>> table(n, std::vector<bool>(n, false));

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            bool iAccepts = acceptStates.find(states[i]) != acceptStates.end();
            bool jAccepts = acceptStates.find(states[j]) != acceptStates.end();
            if (iAccepts != jAccepts) { table[i][j] = true; table[j][i] = true; }
        }
    }

    bool changed;
    do {
        changed = false;
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                if (!table[i][j]) {
                    for (const std::string& sym : alphabet) {
                        auto itI = transitions.find({states[i], sym});
                        auto itJ = transitions.find({states[j], sym});
                        if (itI != transitions.end() && itJ != transitions.end()) {
                            std::string nextI = itI->second;
                            std::string nextJ = itJ->second;
                            int idxI = std::distance(states.begin(), std::find(states.begin(), states.end(), nextI));
                            int idxJ = std::distance(states.begin(), std::find(states.begin(), states.end(), nextJ));
                            if (table[idxI][idxJ]) {
                                table[i][j] = true; table[j][i] = true; changed = true; break;
                            }
                        }
                    }
                }
            }
        }
    } while (changed);

    std::map<std::string, std::string> representative;
    for (int i = 0; i < n; ++i) representative[states[i]] = states[i];

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (!table[i][j]) {
                std::string rootI = representative[states[i]];
                std::string rootJ = representative[states[j]];
                if (rootI != rootJ) {
                    std::string newRoot = (rootI < rootJ) ? rootI : rootJ;
                    for (auto& pair : representative) {
                        if (pair.second == rootI || pair.second == rootJ) pair.second = newRoot;
                    }
                }
            }
        }
    }

    std::set<std::string> minimalStates;
    for (const auto& s : states) minimalStates.insert(representative[s]);

    std::map<std::string, std::string> newNames;
    for (const std::string& state : minimalStates) {
        std::string numPart = state.substr(1);
        newNames[state] = "s" + numPart;
    }

    std::vector<std::string> outStates;
    for (const std::string& state : minimalStates) outStates.push_back(newNames[state]);
    std::sort(outStates.begin(), outStates.end());

    std::string outStart = newNames[representative[startState]];
    std::set<std::string> outAcceptSet;
    for (const std::string& accept : acceptStates) outAcceptSet.insert(newNames[representative[accept]]);
    std::vector<std::string> outAccepts(outAcceptSet.begin(), outAcceptSet.end());
    std::sort(outAccepts.begin(), outAccepts.end());

    struct Edge {
        std::string from; std::string sym; std::string to;
        bool operator<(const Edge& other) const {
            if (from != other.from) return from < other.from;
            if (sym != other.sym) return sym < other.sym;
            return to < other.to;
        }
    };
    std::set<Edge> minimalEdgesSet;
    for (const auto& trans : transitions) {
        std::string fromOrig = trans.first.first;
        std::string sym = trans.first.second;
        std::string toOrig = trans.second;
        std::string fromNew = newNames[representative[fromOrig]];
        std::string toNew = newNames[representative[toOrig]];
        minimalEdgesSet.insert({fromNew, sym, toNew});
    }

    std::vector<Edge> outEdges(minimalEdgesSet.begin(), minimalEdgesSet.end());
    std::sort(outEdges.begin(), outEdges.end());

    std::ofstream outputFile(outputFilename);
    if (!outputFile) return 1;

    for (size_t i = 0; i < outStates.size(); ++i) outputFile << outStates[i] << (i + 1 < outStates.size() ? " " : "");
    outputFile << "\n";
    size_t idx = 0;
    for (const std::string& sym : validAlphabet) {
        outputFile << sym << (idx + 1 < validAlphabet.size() ? " " : ""); idx++;
    }
    outputFile << "\n" << outStart << "\n";
    for (size_t i = 0; i < outAccepts.size(); ++i) outputFile << outAccepts[i] << (i + 1 < outAccepts.size() ? " " : "");
    outputFile << "\n";
    for (size_t i = 0; i < outEdges.size(); ++i) {
        outputFile << outEdges[i].from << " " << outEdges[i].sym << " " << outEdges[i].to;
        if (i + 1 < outEdges.size()) outputFile << "\n";
    }

    outputFile.close();
    return 0;
}

// ==================== 4. FELADAT: REGEXEK  ====================
int DfaProblem::run_rgx(const cxxopts::ParseResult &args) {
    int task_num = args["rgx"].as<int>();
    std::string inputFilename = args["input"].as<std::string>();
    std::string outputFilename = args["output"].as<std::string>();

    std::ifstream inputFile(inputFilename);
    if (!inputFile) return 1;

    std::ofstream outputFile(outputFilename);
    if (!outputFile) return 1;

    std::string line;
    // Logcat sorok szétdarabolására szolgáló Regex
    std::regex log_regex(R"(^([0-9-]+)\s+([0-9:.]+)\s+([0-9]+)\s+([0-9]+)\s+([IDVEFAW])\s+(.+?):\s*(.*)$)");
    std::smatch m;

    if (task_num == 1) {
        int c1 = 0, c2 = 0, c3 = 0, c4 = 0, c5 = 0, c6 = 0, c7 = 0, c8 = 0, c9 = 0, c10 = 0;
        
        // Szigorú regex_match helyett regex_search-re (tartalmazásra) optimalizált minták
        std::regex rx1("10-15");
        std::regex rx2("10:18:51");
        std::regex rx3(R"(\d{2}:\d{2}:51)");
        std::regex rx4("lowmemorykiller");
        std::regex rx5("^221$"); // PID pontos egyezése
        std::regex rx6("^E$");
        std::regex rx7("^W$");
        std::regex rx8("PackageManager");
        std::regex rx9("^D$");
        std::regex rx10("ExoPlayer");
        std::regex rx11(R"(\.java:)");
        std::regex rx12("Thread");

        while (std::getline(inputFile, line)) {
            if (std::regex_search(line, m, log_regex)) {
                std::string date = m[1].str();
                std::string time = m[2].str();
                std::string pid = m[3].str();
                std::string level = m[5].str();
                std::string tag = m[6].str();
                std::string msg = m[7].str();

                // regex_search ("tartalmazza") használata
                if (std::regex_search(date, rx1)) c1++;
                if (std::regex_search(date, rx1) && std::regex_search(time, rx2)) c2++;
                if (std::regex_search(time, rx3)) c3++;
                if (std::regex_search(tag, rx4)) c4++;
                if (std::regex_search(time, rx2) && std::regex_search(pid, rx5)) c5++;
                if (std::regex_search(level, rx6)) c6++;
                if (std::regex_search(level, rx7) && std::regex_search(tag, rx8)) c7++;
                if (std::regex_search(level, rx9) && std::regex_search(tag, rx10)) c8++;
                if ((std::regex_search(level, rx6) || std::regex_search(level, rx7)) && std::regex_search(msg, rx11)) c9++;
                if (std::regex_search(msg, rx12)) c10++;
            }
        }
        outputFile << c1 << "\n" << c2 << "\n" << c3 << "\n" << c4 << "\n" << c5 << "\n"
                   << c6 << "\n" << c7 << "\n" << c8 << "\n" << c9 << "\n" << c10 << "\n";
    }
    else if (task_num == 2) {
        std::regex file_regex(R"(([a-zA-Z0-9_$-]+\.java))");
        while (std::getline(inputFile, line)) {
            if (std::regex_search(line, m, log_regex)) {
                if (m[5].str() == "E") {
                    std::string msg = m[7].str();
                    std::smatch match_file;
                    if (std::regex_search(msg, match_file, file_regex)) {
                        outputFile << match_file[1].str() << "\n";
                    }
                }
            }
        }
    }
    else if (task_num == 3) {
        std::vector<std::string> order;
        std::set<std::string> seen;
        while (std::getline(inputFile, line)) {
            if (std::regex_search(line, m, log_regex)) {
                std::string pid = m[3].str();
                if (seen.find(pid) == seen.end()) {
                    seen.insert(pid);
                    order.push_back(pid);
                }
            }
        }
        for (const auto& p : order) outputFile << p << "\n";
    }
    else if (task_num == 4) {
        std::map<std::string, int> err_counts;
        std::vector<std::string> order;
        while (std::getline(inputFile, line)) {
            if (std::regex_search(line, m, log_regex)) {
                if (m[5].str() == "E") {
                    std::string pid = m[3].str();
                    if (err_counts.find(pid) == err_counts.end()) order.push_back(pid);
                    err_counts[pid]++;
                }
            }
        }
        std::string max_pid = "";
        int max_err = -1;
        for (const auto& p : order) {
            if (err_counts[p] > max_err) {
                max_err = err_counts[p];
                max_pid = p;
            }
        }
        if (max_err != -1) outputFile << "PID=" << max_pid << " (" << max_err << " errors)\n";
    }
    else if (task_num == 5) {
        int main_thread_errors = 0;
        while (std::getline(inputFile, line)) {
            if (std::regex_search(line, m, log_regex)) {
                if (m[5].str() == "E" && m[3].str() == m[4].str()) {
                    main_thread_errors++;
                }
            }
        }
        outputFile << main_thread_errors << "\n";
    }

    outputFile.close();
    return 0;
}