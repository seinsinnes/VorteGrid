cmake_minimum_required(VERSION 3.15)
project(MjgIntelFluidDemo_Part20 CXX)

# Require C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release build if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# Find all .cpp files in this directory and subdirectories
# Note: While GLOB_RECURSE mirrors your Makefile's `find` command, 
# explicitly listing source files is generally best practice in CMake.
file(GLOB_RECURSE SRCS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Create the executable
add_executable(${PROJECT_NAME} ${SRCS})

# Common Include Directories and Flags
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(${PROJECT_NAME} PRIVATE -Wall)

# Common Link Libraries (-lm)
target_link_libraries(${PROJECT_NAME} PRIVATE m)

# ------------------------------------------------------------------------------
# Build Configuration Specific Flags
# ------------------------------------------------------------------------------

# Debug Configuration
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    $<$<CONFIG:Debug>:_DEBUG>
)
target_compile_options(${PROJECT_NAME} PRIVATE 
    $<$<CONFIG:Debug>:-O0 -g -fsanitize=address>
)
target_link_options(${PROJECT_NAME} PRIVATE 
    $<$<CONFIG:Debug>:-fsanitize=address>
)

# Release Configuration
target_compile_options(${PROJECT_NAME} PRIVATE 
    $<$<CONFIG:Release>:-O3 -ffast-math>
)

# ------------------------------------------------------------------------------
# OS Specific Flags
# ------------------------------------------------------------------------------

if(APPLE)
    # macOS specific
    target_compile_options(${PROJECT_NAME} PRIVATE -fdeclspec)
    target_include_directories(${PROJECT_NAME} PRIVATE /opt/homebrew/include)
    target_link_directories(${PROJECT_NAME} PRIVATE /opt/homebrew/lib)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework OpenGL" "-framework GLUT")
elseif(UNIX AND NOT APPLE)
    # Linux (openSUSE) specific
    target_link_libraries(${PROJECT_NAME} PRIVATE GL GLU glut)
endif()
