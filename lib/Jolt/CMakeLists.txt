cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(JoltPhysics VERSION 5.4.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_ALL_WARNINGS "Enable all warnings" ON)
option(INTERPROCEDURAL_OPTIMIZATION "Enable LTO" ON)

# -------- Gather all .cpp source files --------
file(GLOB_RECURSE JOLT_PHYSICS_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/AABBTree/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Geometry/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Math/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ObjectStream/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Physics/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Renderer/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Skeleton/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TriangleSplitter/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/RegisterTypes.cpp"
)

message(STATUS "Found ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Collected ${JOLT_PHYSICS_SRC_FILES}")

# -------- Compiler / Warning settings --------
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

    add_compile_options(/Zc:__cplusplus /MP /nologo /fp:fast)
    if (ENABLE_ALL_WARNINGS)
        add_compile_options(/W4)
    endif()
else()
    if (ENABLE_ALL_WARNINGS)
        add_compile_options(-Wall -Wextra -Wpedantic)
    endif()
endif()

# -------- Interprocedural optimization (LTO) --------
if (INTERPROCEDURAL_OPTIMIZATION)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT LTO_SUPPORTED OUTPUT LTO_CHECK)
    if (LTO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        message(STATUS "LTO enabled")
    endif()
endif()

# -------- Create static library --------
add_library(Jolt STATIC ${JOLT_PHYSICS_SRC_FILES})
# Add both the current folder (for "Jolt.h") and a virtual "Jolt/" prefix include
target_include_directories(Jolt PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}          # So it finds Jolt.h
    ${CMAKE_CURRENT_SOURCE_DIR}/..       # So "Jolt/Jolt.h" works
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

message(STATUS "Configured minimal Jolt build (static lib only)")
