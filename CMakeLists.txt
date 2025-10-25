cmake_minimum_required(VERSION 3.14...3.22)

project(
        QuantDreamCpp
        VERSION 1.0
        LANGUAGES CXX
)

# ========================
# Build Settings & Guards
# ========================
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed. Please build in a separate directory.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ========================
# Dependency Management
# ========================
include(cmake/CPM.cmake)

# PackageProject (for versioned install/export support)
CPMAddPackage(
        NAME PackageProject
        GITHUB_REPOSITORY TheLartians/PackageProject.cmake
        GIT_TAG v1.8.0
)

# fmt
CPMAddPackage(
        NAME fmt
        GITHUB_REPOSITORY fmtlib/fmt
        GIT_TAG 10.2.1
        OPTIONS "FMT_INSTALL YES"
)

# External libraries
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

# ========================
# External Submodules
# ========================
add_subdirectory(external/IBapi)
add_subdirectory(external/IBWrapper)

# ========================
# Core Library
# ========================
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

add_library(${PROJECT_NAME} ${headers} ${sources})
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

# --- Include directories ---
target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external/IBWrapper/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external/IBapi/source/cppclient>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external/IBapi/source/cppclient/client>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external/IBapi/source/proto>
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)
# ========================
# Protobuf & Abseil
# ========================
find_package(Protobuf REQUIRED)
find_package(absl CONFIG REQUIRED)
message(STATUS "Using Abseil from: ${absl_DIR}")
message(STATUS "Using Protobuf from: ${Protobuf_LIBRARY}")

# Link all relevant Abseil components used by Protobuf >= 20240722
set(ABSL_LIBS
        absl::base
        absl::log
        absl::log_internal_check_op
        absl::hash
        absl::strings
        absl::str_format
        absl::time
        absl::cord
)

# ========================
# Intel Decimal Math Library (Linux & macOS)
# ========================
find_library(BID_LIB bid HINTS /usr/local/lib)
if (NOT BID_LIB)
  message(FATAL_ERROR "libbid.a not found in /usr/local/lib")
endif()
message(STATUS "Using Intel Decimal Math library: ${BID_LIB}")

# --- Link everything ---
target_link_libraries(${PROJECT_NAME}
        PRIVATE fmt::fmt
        PRIVATE CURL::libcurl
        PUBLIC Eigen3::Eigen
        PUBLIC ibapi
        PRIVATE IBWrapper
        PRIVATE ${BID_LIB}
        PUBLIC ${ABSL_LIBS}
        PUBLIC ${Protobuf_LIBRARIES}
)

if(HAS_ORTOOLS)
  target_link_libraries(${PROJECT_NAME} PUBLIC ortools::ortools)
  target_compile_definitions(${PROJECT_NAME} PUBLIC HAS_ORTOOLS=1)
else()
  target_compile_definitions(${PROJECT_NAME} PUBLIC HAS_ORTOOLS=0)
endif()

# ========================
# Helper Function for Executables
# ========================
function(add_quant_executable name src)
  add_executable(${name} ${src})
  target_link_libraries(${name} PRIVATE QuantDreamCpp)
endfunction()

# ========================
# Executables
# ========================

## Gaterhing Data
add_quant_executable(AlphaVantageGetData standalone/source/legacy/alphaVantageGetData.cpp)
add_quant_executable(loadYFCSV standalone/source/legacy/loadYFCSV.cpp)
## Monte Carlo Simulations
add_quant_executable(MonteCarloSimulation standalone/source/legacy/MonteCarloSimulation.cpp)
add_quant_executable(RollingMonteCarloSimulation standalone/source/legacy/RollingMonteCarloSimulation.cpp)
add_quant_executable(MonteCarloComparison standalone/source/legacy/comparison.cpp)
## IBKR Related
add_quant_executable(IBKR_get_market_data standalone/source/ibkr/get_market_data.cpp)
## Testing
add_quant_executable(trimmed_mean_test test/source/statistics/robust/center/trimmed_mean.cpp)
add_quant_executable(winsorized_mean_test test/source/statistics/robust/center/winsorized_mean.cpp)