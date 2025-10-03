cmake_minimum_required(VERSION 3.22)

project(VectorVault 
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "Blazing-fast ANN vector search engine with HNSW + SIMD"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(VECTORVAULT_ENABLE_AVX2 "Enable AVX2 SIMD optimizations" ON)
option(VECTORVAULT_WERROR "Treat warnings as errors" ON)
option(VECTORVAULT_BUILD_PYBIND "Build Python bindings" OFF)
option(VECTORVAULT_BUILD_TESTS "Build tests" ON)
option(VECTORVAULT_BUILD_BENCH "Build benchmarks" ON)

# Include custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(cmake/Dependencies.cmake)
include(cmake/warnings.cmake)

# Detect AVX2 support
if(VECTORVAULT_ENABLE_AVX2)
    include(CheckCXXCompilerFlag)
    if(MSVC)
        check_cxx_compiler_flag("/arch:AVX2" COMPILER_SUPPORTS_AVX2)
        set(AVX2_FLAGS "/arch:AVX2")
    else()
        check_cxx_compiler_flag("-mavx2 -mfma" COMPILER_SUPPORTS_AVX2)
        set(AVX2_FLAGS "-mavx2 -mfma")
    endif()
    
    if(COMPILER_SUPPORTS_AVX2)
        message(STATUS "AVX2 support detected and enabled")
        add_compile_definitions(VECTORVAULT_ENABLE_AVX2)
    else()
        message(WARNING "AVX2 requested but not supported by compiler")
        set(VECTORVAULT_ENABLE_AVX2 OFF)
    endif()
endif()

# Core library
add_library(vectorvault_lib STATIC
    src/hnsw.cpp
    src/distance_avx2.cpp
    src/distance_naive.cpp
    src/thread_pool.cpp
    src/mmap_io.cpp
    src/index_factory.cpp
)

target_include_directories(vectorvault_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(vectorvault_lib PUBLIC
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

# Apply warnings
set_project_warnings(vectorvault_lib)

# AVX2 for specific files
if(VECTORVAULT_ENABLE_AVX2)
    set_source_files_properties(src/distance_avx2.cpp PROPERTIES COMPILE_FLAGS "${AVX2_FLAGS}")
endif()

# REST API server
add_executable(vectorvault_api
    api/main.cpp
)

target_link_libraries(vectorvault_api PRIVATE
    vectorvault_lib
    httplib::httplib
)

set_project_warnings(vectorvault_api)

# Benchmarks
if(VECTORVAULT_BUILD_BENCH)
    add_executable(vectorvault_bench
        bench/bench_main.cpp
        bench/gen_data.cpp
        bench/brute_force.cpp
    )
    
    target_link_libraries(vectorvault_bench PRIVATE
        vectorvault_lib
    )
    
    set_project_warnings(vectorvault_bench)
endif()

# Tests
if(VECTORVAULT_BUILD_TESTS)
    enable_testing()
    
    add_executable(vectorvault_tests
        tests/test_distance.cpp
        tests/test_hnsw_small.cpp
        tests/test_persistence.cpp
        tests/test_api_integration.cpp
    )
    
    target_link_libraries(vectorvault_tests PRIVATE
        vectorvault_lib
        httplib::httplib
        GTest::gtest
        GTest::gtest_main
    )
    
    set_project_warnings(vectorvault_tests)
    
    include(GoogleTest)
    gtest_discover_tests(vectorvault_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTIES TIMEOUT 300
    )
endif()

# Custom targets for formatting and linting
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_CXX_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/api/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/api/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/bench/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
    )
    
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${ALL_CXX_FILES}
        COMMENT "Running clang-format on all source files"
    )
endif()

find_program(CLANG_TIDY clang-tidy)
if(CLANG_TIDY)
    add_custom_target(tidy
        COMMAND ${CLANG_TIDY} -p ${CMAKE_BINARY_DIR} ${ALL_CXX_FILES}
        COMMENT "Running clang-tidy"
    )
endif()

# Installation rules
include(GNUInstallDirs)

install(TARGETS vectorvault_lib vectorvault_api
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/vectorvault
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Python bindings (optional)
if(VECTORVAULT_BUILD_PYBIND)
    add_subdirectory(python)
endif()
