cmake_minimum_required(VERSION 3.14...3.22)

project(
        QuantDreamCpp
        VERSION 1.0
        LANGUAGES CXX
)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR "In-source builds not allowed.")
endif()

include(cmake/CPM.cmake)

CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.8.0")

CPMAddPackage(
        NAME fmt
        GIT_TAG 10.2.1
        GITHUB_REPOSITORY fmtlib/fmt
        OPTIONS "FMT_INSTALL YES"
)

# sudo apt-get install libabsl-dev

find_package(CURL REQUIRED)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(ortools REQUIRED CONFIG)

file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

add_library(${PROJECT_NAME} ${headers} ${sources})
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE fmt::fmt
        PUBLIC Eigen3::Eigen
        PUBLIC ortools::ortools
)

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

add_executable(AlphaVantageGetData standalone/source/alphaVantageGetData.cpp)
target_link_libraries(AlphaVantageGetData PRIVATE QuantDreamCpp CURL::libcurl)

add_executable(loadYFCSV standalone/source/loadYFCSV.cpp)
target_link_libraries(loadYFCSV PRIVATE QuantDreamCpp CURL::libcurl)

add_executable(MonteCarloSimulation standalone/source/MonteCarloSimulation.cpp)
target_link_libraries(MonteCarloSimulation PRIVATE QuantDreamCpp CURL::libcurl)
