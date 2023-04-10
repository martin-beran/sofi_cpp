# Link Boost::Test library to test programs
link_libraries(
    soficpp
    Threads::Threads
)

set(
    TEST_PROGRAMS
    agent
    dummy_boost
    engine
    entity
    enum_str
    integrity
)

if (TEST_COVERAGE)
    add_custom_target(coverage)
    add_custom_target(run_coverage)
endif()

set(PROFBIN "")
set(PROFRAW "")
foreach (P ${TEST_PROGRAMS})
    set(PB "${CMAKE_CURRENT_BINARY_DIR}/test_${P}")
    set(PR "${PB}.profraw")
    list(APPEND PROFRAW ${PR})
    list(APPEND PROFBIN -object ${PB})
    if (TEST_COVERAGE)
        add_custom_target(coverage_${P} COMMAND LLVM_PROFILE_FILE=${PR} ${PB})
        add_dependencies(run_coverage coverage_${P})
    endif()
    add_executable(test_${P} test_${P}.cpp)
    add_test(NAME ${P} COMMAND test_${P})
endforeach()

if (TEST_COVERAGE)
    add_custom_target(
        report_coverage
        COMMAND llvm-profdata merge -sparse -o ${CMAKE_CURRENT_BINARY_DIR}/test.profraw ${PROFRAW}
        COMMAND llvm-cov show -format=html -instr-profile=test.profraw -Xdemangler=c++filt -Xdemangler=-n
            -output-dir=${CMAKE_CURRENT_BINARY_DIR}/coverage_html ${PROFBIN}
    )
    add_dependencies(coverage report_coverage)
    add_dependencies(report_coverage run_coverage)
endif()
