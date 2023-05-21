add_library(soficpp INTERFACE)
target_include_directories(soficpp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Build C++ SQLite3 API library
add_library(sqlite_cpp sqlite_cpp.cpp)
target_link_libraries(sqlite_cpp SQLite::SQLite3)

# Build program sofi_demo
add_executable(sofi_demo sofi_demo.cpp)
target_link_libraries(sofi_demo sqlite_cpp)
