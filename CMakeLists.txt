cmake_minimum_required(VERSION 3.10)

# Set the project name
project(ThreadPoolProject)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the build type to Debug
set(CMAKE_BUILD_TYPE Debug)

# Generate compile_commands.json for IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add the executable
add_executable(ThreadPoolExecutable main.cpp data.cpp dataset.cpp frequency.cpp tokeniser.cpp threadpool.cpp args.cpp)
target_include_directories(ThreadPoolExecutable PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link pthread library
find_package(Threads REQUIRED)
target_link_libraries(ThreadPoolExecutable PRIVATE Threads::Threads)

# Link OpenMP library
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    target_link_libraries(ThreadPoolExecutable PRIVATE OpenMP::OpenMP_CXX)
endif()