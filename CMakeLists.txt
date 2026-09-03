# A range is required: LLVM's exported CMake config carries legacy policy
# settings that fail to configure under CMake 4.x with an upper-bound-only
# minimum.
cmake_minimum_required(VERSION 3.20...4.1)

project(cxx_safety
        VERSION 0.1.0
        DESCRIPTION "A unified memory and thread safety analyzer for C/C++"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM  ${LLVM_PACKAGE_VERSION}")
message(STATUS "  LLVMConfig.cmake  : ${LLVM_DIR}")
message(STATUS "  ClangConfig.cmake : ${Clang_DIR}")
message(STATUS "  LLVM_ENABLE_RTTI  : ${LLVM_ENABLE_RTTI}")

# Shared build settings for every target in this project.
add_library(cxx_safety_options INTERFACE)

target_include_directories(cxx_safety_options SYSTEM INTERFACE
        ${LLVM_INCLUDE_DIRS}
        ${CLANG_INCLUDE_DIRS})

target_include_directories(cxx_safety_options INTERFACE
        ${PROJECT_SOURCE_DIR}/include)

# LLVM_DEFINITIONS is a single space-separated string, not a list.
separate_arguments(CXX_SAFETY_LLVM_DEFS NATIVE_COMMAND "${LLVM_DEFINITIONS}")
target_compile_options(cxx_safety_options INTERFACE ${CXX_SAFETY_LLVM_DEFS})

# LLVM is normally built without RTTI. Linking against it with RTTI enabled
# produces undefined typeinfo symbols.
if(NOT LLVM_ENABLE_RTTI)
    if(MSVC)
        target_compile_options(cxx_safety_options INTERFACE /GR-)
    else()
        target_compile_options(cxx_safety_options INTERFACE -fno-rtti)
    endif()
endif()

if(MSVC)
    target_compile_options(cxx_safety_options INTERFACE /W4 /permissive-)
else()
    target_compile_options(cxx_safety_options INTERFACE
            -Wall -Wextra -Wpedantic)
endif()

add_subdirectory(src)
