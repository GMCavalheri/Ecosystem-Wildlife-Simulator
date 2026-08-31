cmake_minimum_required(VERSION 3.21)
project(EcosystemWildlifeSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(EnTT CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_library(ecosystem_core
    src/core/Simulation.cpp
    src/core/MetricsRecorder.cpp
    src/environment/Grid.cpp
    src/systems/EnvironmentSystem.cpp
    src/systems/ForagingSystem.cpp
    src/systems/PredationSystem.cpp
    src/systems/ReproductionSystem.cpp
    src/systems/DiseaseSystem.cpp
    src/systems/MigrationSystem.cpp
    src/systems/MortalitySystem.cpp
)

target_include_directories(ecosystem_core PUBLIC src)
target_compile_features(ecosystem_core PUBLIC cxx_std_20)
target_link_libraries(ecosystem_core PUBLIC EnTT::EnTT spdlog::spdlog)

add_executable(ecosystem_sim src/main.cpp)
target_link_libraries(ecosystem_sim PRIVATE ecosystem_core)

enable_testing()
add_subdirectory(tests)
