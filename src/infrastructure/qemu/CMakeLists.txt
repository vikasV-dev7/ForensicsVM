include(FetchContent)
FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  TLS_VERIFY OFF
)
FetchContent_MakeAvailable(json)

add_library(QemuInfrastructure STATIC
    QemuLocator.cpp
    QemuCommandBuilder.cpp
    QemuProcess.cpp
    QemuBackend.cpp
    QmpClient.cpp
)


target_include_directories(QemuInfrastructure PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(QemuInfrastructure PUBLIC VmDomain PUBLIC nlohmann_json::nlohmann_json)
