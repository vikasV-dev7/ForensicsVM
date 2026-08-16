add_library(QemuInfrastructure STATIC
    QemuLocator.cpp
    QemuCommandBuilder.cpp
    QemuProcess.cpp
    QemuBackend.cpp
)

target_include_directories(QemuInfrastructure PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(QemuInfrastructure PUBLIC VmDomain)
