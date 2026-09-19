find_package(Catch2 3 CONFIG REQUIRED)

add_executable(ecosystem_tests
    test_grid.cpp
    test_simulation.cpp
    test_vegetation.cpp
    test_population_dynamics.cpp
    test_genetics.cpp
    test_migration.cpp
    test_predator_resilience.cpp
    test_disease.cpp
    test_weighted_sampling.cpp
    test_parallel.cpp
    test_biomes.cpp
    test_seasonal_migration.cpp
)

target_link_libraries(ecosystem_tests PRIVATE ecosystem_core Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(ecosystem_tests)
