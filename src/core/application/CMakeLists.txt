add_library(CoreApplication STATIC
    services/ForensicApplicationImpl.cpp
)
target_include_directories(CoreApplication PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(CoreApplication PUBLIC VmManagement)

add_library(ApplicationBootstrap ApplicationBootstrap.cpp)
target_include_directories(ApplicationBootstrap PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(ApplicationBootstrap PUBLIC VmManagement InMemoryInfrastructure fvm_crypto CoreApplication SqliteInfrastructure QemuInfrastructure)
