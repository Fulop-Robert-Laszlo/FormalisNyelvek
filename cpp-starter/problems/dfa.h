#ifndef PROBLEMS_DFA_H
#define PROBLEMS_DFA_H

#include "../problem.hpp"

class DfaProblem : public Problem {
public:
    void initialize_parser(cxxopts::Options &options) override;
    bool is_chosen_problem(const cxxopts::ParseResult &args) override;
    int run(const cxxopts::ParseResult &args) override;

private:
    int run_check(const cxxopts::ParseResult &args);
    int run_det(const cxxopts::ParseResult &args);
    int run_mini(const cxxopts::ParseResult &args);
    int run_rgx(const cxxopts::ParseResult &args);
};

#endif // PROBLEMS_DFA_H