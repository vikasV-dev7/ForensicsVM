add_executable(sanity_test sanity_test.cpp)
add_test(NAME SanityTest COMMAND sanity_test)

add_executable(VmIdTest VmIdTest.cpp)
target_link_libraries(VmIdTest PRIVATE VmDomain)
add_test(NAME VmIdTest COMMAND VmIdTest)

add_executable(VmConfigTest VmConfigTest.cpp)
target_link_libraries(VmConfigTest PRIVATE VmDomain)
add_test(NAME VmConfigTest COMMAND VmConfigTest)

add_executable(VmManagerTest VmManagerTest.cpp)
target_link_libraries(VmManagerTest PRIVATE VmManagement InMemoryInfrastructure)
add_test(NAME VmManagerTest COMMAND VmManagerTest)

add_executable(CompositionTest CompositionTest.cpp)
target_link_libraries(CompositionTest PRIVATE ApplicationBootstrap VmManagement)
add_test(NAME CompositionTest COMMAND CompositionTest)
