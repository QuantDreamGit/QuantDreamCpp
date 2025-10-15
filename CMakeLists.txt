cmake_minimum_required(VERSION 3.14...3.22)

project(
        QuantDreamCpp
        VERSION 1.0
        LANGUAGES CXX
)

# ---- Include guards ----
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR "In-source builds not allowed.")
endif()

# ---- Dependency Management ----
include(cmake/CPM.cmake)

CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.8.0")

CPMAddPackage(
        NAME fmt
        GIT_TAG 10.2.1
        GITHUB_REPOSITORY fmtlib/fmt
        OPTIONS "FMT_INSTALL YES"
)

# ---- External Libraries ----
find_package(CURL REQUIRED)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)

# Optional OR-Tools
find_package(ortools CONFIG QUIET)
if(ortools_FOUND)
  message(STATUS "Found OR-Tools: ${ortools_DIR}")
  set(HAS_ORTOOLS TRUE)
else()
  message(WARNING "OR-Tools not found. Optimization features will be disabled.")
  set(HAS_ORTOOLS FALSE)
endif()

# ---- External Submodules ----
add_subdirectory(external/IBapi)
add_subdirectory(external/IBWrapper)

# ---- Core Library ----
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

add_library(${PROJECT_NAME} ${headers} ${sources})
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Linking ----
target_link_libraries(${PROJECT_NAME}
        PRIVATE fmt::fmt
        PUBLIC Eigen3::Eigen
        PRIVATE IBWrapper
)

if(HAS_ORTOOLS)
  target_link_libraries(${PROJECT_NAME} PUBLIC ortools::ortools)
  target_compile_definitions(${PROJECT_NAME} PUBLIC HAS_ORTOOLS=1)
else()
  target_compile_definitions(${PROJECT_NAME} PUBLIC HAS_ORTOOLS=0)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# ---- Standalone Executables ----
add_executable(AlphaVantageGetData standalone/source/legacy/alphaVantageGetData.cpp)
target_link_libraries(AlphaVantageGetData PRIVATE QuantDreamCpp CURL::libcurl)

add_executable(loadYFCSV standalone/source/legacy/loadYFCSV.cpp)
target_link_libraries(loadYFCSV PRIVATE QuantDreamCpp CURL::libcurl)

add_executable(MonteCarloSimulation standalone/source/legacy/MonteCarloSimulation.cpp)
target_link_libraries(MonteCarloSimulation PRIVATE QuantDreamCpp CURL::libcurl)

add_executable(RollingMonteCarloSimulation standalone/source/legacy/RollingMonteCarloSimulation.cpp)
target_link_libraries(RollingMonteCarloSimulation PRIVATE QuantDreamCpp CURL::libcurl)

add_executable(MonteCarloComparison standalone/source/legacy/comparison.cpp)
target_link_libraries(MonteCarloComparison PRIVATE QuantDreamCpp CURL::libcurl)

add_executable(trimmed_mean_test test/source/statistics/robust/center/trimmed_mean.cpp)
target_link_libraries(trimmed_mean_test PRIVATE QuantDreamCpp CURL::libcurl)

add_executable(winsorized_mean_test test/source/statistics/robust/center/winsorized_mean.cpp)
target_link_libraries(winsorized_mean_test PRIVATE QuantDreamCpp CURL::libcurl)

# ---- Test executable using IBWrapper ----
add_executable(test test/source/main_test.cpp)
target_link_libraries(test PRIVATE QuantDreamCpp CURL::libcurl)
target_include_directories(test PRIVATE
        ${PROJECT_SOURCE_DIR}/external/IBWrapper/include
        ${PROJECT_SOURCE_DIR}/external/IBapi/source/cppclient
        ${PROJECT_SOURCE_DIR}/external/IBapi/source/cppclient/client
        ${PROJECT_SOURCE_DIR}/external/IBapi/source/proto
)