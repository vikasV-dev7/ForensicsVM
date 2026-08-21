cmake_minimum_required(VERSION 3.20)

project(ForensicVMDesktop)

find_package(Qt6 COMPONENTS Core Gui Qml Quick REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(ForensicVMDesktop WIN32
    main.cpp
    resources.qrc
)

target_link_libraries(ForensicVMDesktop
    PRIVATE
        PresentationLayer
        CoreApplication
        ApplicationBootstrap
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
)

target_include_directories(ForensicVMDesktop
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(ForensicVMDesktop PRIVATE /W4 /WX)
else()
    target_compile_options(ForensicVMDesktop PRIVATE -Wall -Wextra -Werror -Wpedantic)
endif()
