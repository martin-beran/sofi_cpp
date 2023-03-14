# Link Boost::Test library to test programs
link_libraries(
    Threads::Threads
)

set(
    TEST_PROGRAMS
    dummy_boost
)

foreach (P ${TEST_PROGRAMS})
    add_executable(test_${P} test_${P}.cpp)
    add_test(NAME ${P} COMMAND test_${P})
endforeach()
