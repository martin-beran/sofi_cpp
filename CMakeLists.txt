cmake_minimum_required(VERSION 3.22.1)

project(SofiCpp LANGUAGES CXX)

include(CTest)

# Known build types
set(
    BUILD_TYPES
    Debug
    Release
    RelWithDebInfo
    MinSizeRel
)

# Known sanitizers
set(
    SANITIZERS
    OFF
    address
    leak
    thread
    undefined
)

### BEGIN Build configuration #################################################
option(USE_LIBC++ "Use libc++ instead of libstdc++" OFF)
option(USE_LTO "Use LTO (link time, interprocedural optimization)" ON)
option(TEST_COVERAGE "Build and run tests with coverage analysis" OFF)

set(
    BUILD_TYPE "RelWithDebInfo" CACHE STRING
    "Select a build type (${BUILD_TYPES})"
)

set(SANITIZER "OFF" CACHE STRING "Select a sanitizer (${SANITIZERS})")

### END Build configuration ###################################################

# Check build configuration
set_property(CACHE BUILD_TYPE PROPERTY STRINGS ${BUILD_TYPES})
if (NOT BUILD_TYPE IN_LIST BUILD_TYPES)
    message(FATAL_ERROR "Unknown value of BUILD_TYPE: ${BUILD_TYPE}")
endif()
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE ${BUILD_TYPE})
endif()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${USE_LTO})

if (TEST_COVERAGE AND NOT CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    message(FATAL_ERROR "Coverage analysis available only for Clang")
endif()

set_property(CACHE SANITIZER PROPERTY STRINGS ${SANITIZERS})
if (NOT SANITIZER IN_LIST SANITIZERS)
    message(FATAL_ERROR "Unknown value of SANITIZER: ${SANITIZER}")
endif()

# Version from Git
message(CHECK_START "Getting version by Git")
execute_process(
    COMMAND git describe --always --dirty
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE OUTPUT_STRIP_TRAILING_WHITESPACE
)
if (GIT_DESCRIBE)
    message(CHECK_PASS "OK")
else()
    message(CHECK_FAIL "FAIL")
    set(GIT_DESCRIBE "unknown")
endif()

# C++ options
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
file(REMOVE ${CMAKE_SOURCE_DIR}/compile_commands.json)
file(
    CREATE_LINK
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
    SYMBOLIC
)
add_compile_options(
    -Wall -Wextra -pedantic -Werror
    -Wconversion -Wsign-conversion
    -Wswitch-default -Wswitch-enum
    -Wno-mismatched-new-delete
)
add_compile_definitions(GIT_VERSION="${GIT_DESCRIBE}")
if (USE_LIBC++)
    add_compile_options(-stdlib=libc++)
    add_link_options(-stdlib=libc++)
endif()
if (CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    add_compile_options(-Wimplicit-fallthrough)
endif()
if (CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    add_compile_options(-Wimplicit-fallthrough=5)
endif()
if (TEST_COVERAGE)
    add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
    add_link_options(-fprofile-instr-generate -fcoverage-mapping)
endif()
if (NOT SANITIZER STREQUAL "OFF")
    add_compile_options(
        -fsanitize=${SANITIZER}
    )
    add_link_options(
        -fsanitize=${SANITIZER}
    )
endif()

# We really want asserts in all builds
add_compile_options(-UNDEBUG)

# Used packages
find_package(Boost 1.81 REQUIRED) # Set BOOST_ROOT for an alternative location
find_package(Doxygen 1.9.3 OPTIONAL_COMPONENTS dot)
find_package(SQLite3 3.37 REQUIRED)
find_package(Threads REQUIRED)

include_directories(
    SYSTEM
    ${Boost_INCLUDE_DIRS}
)
link_directories(
    ${Boost_LIBRARY_DIRS}
)

# Optional Markdown utility for generating the top-level documentation stub
find_program(
    MARKDOWN
    NAMES markdown markdown_py
    DOC "Program for processing Markdown to HTML"
)

# Subdirectories with CMake configurations
add_subdirectory(src)
add_subdirectory(test)

# Generate documentation, optional (Markdown)
if (MARKDOWN)
    add_custom_target(doc ALL)
    function(add_markdown FILE)
        set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.md")
        set(DST "${CMAKE_CURRENT_BINARY_DIR}/${FILE}.html")
        cmake_path(GET DST PARENT_PATH DIR)
        file(MAKE_DIRECTORY ${DIR})
        add_custom_command(
            OUTPUT ${DST}
            COMMAND
                sed '1,/^[^[]*$$/s%.Doxygen.: index.html%[Doxygen]: html/index.html%' ${SRC} |
                sed '1,/^[^[]*$$/s%\.md$$%.html%' |
                ${MARKDOWN} > ${DST}
            MAIN_DEPENDENCY ${SRC}
        )
        target_sources(doc PRIVATE ${DST})
        get_target_property(PSRC doc SOURCES)
    endfunction()
    add_markdown(README)
    add_markdown(LICENSE)
    add_markdown(doc/intro)
endif()

# Generate documentation, optional (Doxygen)
if (DOXYGEN_FOUND)
    add_custom_target(
        doxygen
        COMMAND Doxygen::doxygen ${CMAKE_SOURCE_DIR}/Doxyfile > /dev/null
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()

# Report CMake configuration
message(STATUS "=== Build configuration ===")
message(STATUS "Version:      ${GIT_DESCRIBE}")
message(STATUS "Build type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "Markdown:     ${MARKDOWN}")
message(STATUS "Doxygen:      ${DOXYGEN_VERSION}")
message(
    STATUS
    "C++ compiler: ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ID} "
    ${CMAKE_CXX_COMPILER_VERSION}
)
message(STATUS "Use libc++:   ${USE_LIBC++}")
message(STATUS "Use LTO:      ${USE_LTO}")
message(STATUS "Coverage:     ${TEST_COVERAGE}")
message(STATUS "Sanitizer:    ${SANITIZER}")
