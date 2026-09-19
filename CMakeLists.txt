cmake_minimum_required(VERSION 3.21)
project(EcosystemWildlifeSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(EnTT CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_library(ecosystem_core
    src/core/Simulation.cpp
    src/core/MetricsRecorder.cpp
    src/core/WeightedSampling.cpp
    src/core/Persistence.cpp
    src/core/ThreadPool.cpp
    src/environment/Grid.cpp
    src/environment/Biomes.cpp
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
find_package(Threads REQUIRED)
target_link_libraries(ecosystem_core PUBLIC EnTT::EnTT spdlog::spdlog nlohmann_json::nlohmann_json Threads::Threads)

add_executable(ecosystem_sim src/main.cpp)
target_link_libraries(ecosystem_sim PRIVATE ecosystem_core)

# Phase 7: profiling harness (no extra dependencies).
add_executable(ecosystem_bench src/bench/main.cpp)
target_link_libraries(ecosystem_bench PRIVATE ecosystem_core)

# Phase 6: optional viewer, only built when the vcpkg "render" feature is installed
# (cmake --preset default -DVCPKG_MANIFEST_FEATURES=render). The headless core never
# depends on this.
find_package(raylib CONFIG QUIET)
if(raylib_FOUND)
    add_executable(ecosystem_viewer
        src/render/main.cpp
        src/render/Viewer.cpp
    )
    target_link_libraries(ecosystem_viewer PRIVATE ecosystem_core raylib)
else()
    message(STATUS "raylib not found -- skipping ecosystem_viewer (enable the vcpkg 'render' feature to build it)")
endif()

enable_testing()
add_subdirectory(tests)
