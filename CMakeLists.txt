cmake_minimum_required(VERSION 3.20)

project(ForensicVM
    VERSION 0.1.0
    DESCRIPTION "Forensic Virtual Machine Platform"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -Wpedantic)
endif()

add_subdirectory(src/vm)
add_subdirectory(src/infrastructure)
add_subdirectory(src/core)
add_subdirectory(tests)
