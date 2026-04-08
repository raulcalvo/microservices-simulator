cmake_minimum_required(VERSION 3.20)
project(MicroDummy VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)

add_executable(micro_node
    src/main.cpp
)

target_include_directories(micro_node PRIVATE src)

target_link_libraries(micro_node PRIVATE
    zmq
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    cxxopts::cxxopts
)
