add_library(fvm_crypto
    NativeHashCalculator.cpp
)

target_include_directories(fvm_crypto PUBLIC ${CMAKE_SOURCE_DIR}/src)



if(WIN32)
    target_link_libraries(fvm_crypto PUBLIC bcrypt)
endif()
