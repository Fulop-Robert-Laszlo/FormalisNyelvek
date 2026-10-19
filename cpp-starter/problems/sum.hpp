#ifndef PROBLEMS_SUM_H
#define PROBLEMS_SUM_H

#include "../problem.hpp"

class SumProblem : public Problem {
public:
    void initialize_parser(cxxopts::Options &options) override;
    bool is_chosen_problem(const cxxopts::ParseResult &args) override;
    int run(const cxxopts::ParseResult &args) override;
};

#endif // PROBLEMS_SUM_H