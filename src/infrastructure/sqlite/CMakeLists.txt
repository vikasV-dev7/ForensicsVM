include(FetchContent)
FetchContent_Declare(sqlite3_src
  URL https://www.sqlite.org/2023/sqlite-amalgamation-3430200.zip
  URL_HASH SHA256=a17ac8792f57266847d57651c5259001d1e4e4b46be96ec0d985c953925b2a1c
  TLS_VERIFY OFF
)

FetchContent_GetProperties(sqlite3_src)
if(NOT sqlite3_src_POPULATED)
  FetchContent_Populate(sqlite3_src)
endif()

add_library(sqlite3 STATIC 
    ${sqlite3_src_SOURCE_DIR}/sqlite3.c
)
target_include_directories(sqlite3 PUBLIC ${sqlite3_src_SOURCE_DIR})
if(NOT MSVC)
    target_compile_options(sqlite3 PRIVATE -w)
endif()

add_library(SqliteInfrastructure STATIC
    SqliteCaseRepository.cpp
)
target_include_directories(SqliteInfrastructure PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(SqliteInfrastructure PUBLIC CoreApplication sqlite3)
