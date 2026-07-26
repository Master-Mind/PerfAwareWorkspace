cmake_minimum_required(VERSION 3.22)
#using system packages instead now
# Set vcpkg toolchain
#if(DEFINED ENV{VCPKG_ROOT})
#    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
#else()
#    message(FATAL_ERROR "Please set the VCPKG_ROOT environment variable to your vcpkg root directory.")
#endif()

# Set the project name
project(haversine)
enable_language(ASM_NASM)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)

FetchContent_Declare(
        argparse
        GIT_REPOSITORY https://github.com/p-ranav/argparse.git
        GIT_TAG v3.2
)

FetchContent_Declare(
        glaze
        GIT_REPOSITORY https://github.com/stephenberry/glaze.git
        GIT_TAG v7.2.1
)


FetchContent_MakeAvailable(glaze argparse)

# Add source files
file(GLOB SOURCES "*.cpp")
file(GLOB ASMSOURCES "*_lin.asm")

# Add the executable
add_executable(haversine ${SOURCES} ${ASMSOURCES})
target_link_libraries(haversine PRIVATE argparse)
target_link_libraries(haversine PRIVATE glaze::glaze)

# Set the output directory for the executable
set_target_properties(haversine PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)