cmake_minimum_required(VERSION 3.20)

project(PresentationLayer)

find_package(Qt6 COMPONENTS Core REQUIRED)

# Need AUTOMOC to generate moc files for QObject-derived ViewModels
set(CMAKE_AUTOMOC ON)

add_library(PresentationLayer STATIC
    AppViewModel.hpp
    AppViewModel.cpp
    CaseDashboardViewModel.hpp
    CaseDashboardViewModel.cpp
    OperationManagerViewModel.hpp
    OperationManagerViewModel.cpp
)

target_link_libraries(PresentationLayer
    PUBLIC
        CoreApplication
        Qt6::Core
)

target_include_directories(PresentationLayer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(PresentationLayer PRIVATE /W4 /WX)
else()
    target_compile_options(PresentationLayer PRIVATE -Wall -Wextra -Werror -Wpedantic)
endif()
