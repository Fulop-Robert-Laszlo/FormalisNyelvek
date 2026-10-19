cmake_minimum_required(VERSION 3.10)

project(project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add your own problems here
add_executable(project main.cpp problems/sum.cpp
        problems/dfa.cpp
        problems/dfa.h)