add_library(fvm_crypto
    NativeHashCalculator.cpp
)

target_include_directories(fvm_crypto PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(fvm_crypto PUBLIC fvm_contracts)

if(WIN32)
    target_link_libraries(fvm_crypto PUBLIC bcrypt)
endif()
