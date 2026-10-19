#include <string>
#include <fstream>
#include <iostream>

#include "sum.hpp"

// Initialize the parser for the sum problem
void SumProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("sum", "Sum the numbers in the input file");
}

// Check if the sum problem is chosen
bool SumProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("sum") > 0;
}

// Run the sum problem
int SumProblem::run(const cxxopts::ParseResult &args) {
    std::string inputFilename = args["input"].as<std::string>();
    std::string outputFilename = args["output"].as<std::string>();

    std::ifstream inputFile(inputFilename);

    if (!inputFile) {
        std::cerr << "Error opening input file: " << inputFilename << std::endl;
        return 1;
    }

    int sum = 0;
    int number;

    while (inputFile >> number) {
        sum += number;
    }

    std::ofstream outputFile(outputFilename);

    if (!outputFile) {
        std::cerr << "Error opening output file: " << outputFilename << std::endl;
        return 1;
    }

    outputFile << sum;
    return 0;
}