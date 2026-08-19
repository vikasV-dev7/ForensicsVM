add_library(VmManagement VmManager.cpp EvidenceRegistry.cpp)
target_include_directories(VmManagement PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(VmManagement PUBLIC VmDomain)
