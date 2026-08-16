add_subdirectory(application)

add_executable(ForensicVM main.cpp)
target_link_libraries(ForensicVM PRIVATE ApplicationBootstrap)
target_include_directories(ForensicVM PRIVATE ${CMAKE_SOURCE_DIR}/src)
