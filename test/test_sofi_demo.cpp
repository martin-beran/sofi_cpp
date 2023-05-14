/*! \file
 * \brief Tests of program \c sofi_demo
 *
 * Tests in this file check program \c sofi_demo and use this program for
 * testing various functionality of the SOFI C++ library. The tested program is
 * implemented in file sofi_demo.cpp.
 */

//! \cond
#include <cstdlib>
#include <filesystem>

#if __has_include(<sys/types.h>) && __has_include(<sys/wait.h>)
#include <sys/types.h>
#include <sys/wait.h>
#endif

#define BOOST_TEST_MODULE sofi_demo
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

namespace {

/* It uses argv[0] to deduce the path. If arg[0] does not contain '/', it is
 * expected that sofi_demo is stored in a directory in PATH, and the program
 * name without a path is returned. Otherwise, the program is expected to be
 * stored in directory ../src relative to the directory containing
 * test_sofi_demo. */
std::string sofi_demo_exe()
{
    constexpr std::string_view sofi_demo = "sofi_demo";
    std::string argv0 = boost::unit_test::framework::master_test_suite().argv[0];
    if (argv0.find('/') == std::string::npos)
        return std::string{sofi_demo};
    namespace fs = std::filesystem;
    return fs::canonical(fs::path{argv0}.parent_path() / "../src" / sofi_demo);
}

} // namespace
//! \endcond

/*! \file
 * \test \c run -- Program \c sofi_demo can be executed without command line
 * arguments and exits with status 1. */
//! \cond
BOOST_AUTO_TEST_CASE(run)
{
    int status = system((sofi_demo_exe()
#ifdef __unix
                         + " 2> /dev/null"
#endif
                        ).c_str());
    BOOST_TEST(status != 0);
#ifdef WIFEXITED
    BOOST_TEST(WIFEXITED(status));
    BOOST_TEST(WEXITSTATUS(status) == 1);
#endif
}
//! \endcond
