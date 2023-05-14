/*! \file
 * \brief Tests of program \c sofi_demo
 *
 * Tests in this file check program \c sofi_demo and use this program for
 * testing various functionality of the SOFI C++ library. The tested program is
 * implemented in file sofi_demo.cpp.
 *
 * The tests create SQLite database file \c test_sofi_demo.db in the current
 * directory. If the file already exists, it is deleted first.
 */

//! \cond
#include "soficpp/enum_str.hpp"
#include "sqlite_cpp.hpp"

#include <cstdlib>
#include <filesystem>

#if __has_include(<sys/types.h>) && __has_include(<sys/wait.h>)
#include <sys/types.h>
#include <sys/wait.h>
#endif

#define BOOST_TEST_MODULE sofi_demo
#include <boost/test/included/unit_test.hpp>

namespace {

constexpr std::string_view db_file = "test_sofi_demo.db";

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

void sofi_demo_do(std::string_view cmd)
{
    auto exe = sofi_demo_exe();
    exe += ' ';
    exe.append(cmd);
    exe += ' ';
    exe += db_file;
    int status = system(exe.c_str()); // NOLINT(concurrency-mt-unsafe)
    BOOST_TEST_INFO("sofi_demo_do " << cmd);
    BOOST_TEST_REQUIRE(status == 0);
}

void sofi_demo_init()
{
    std::filesystem::remove(db_file);
    sofi_demo_do("init");
}

void sofi_demo_run()
{
    sofi_demo_do("run");
}

struct sofi_test {
    std::vector<std::string> sql_prepare;
    std::vector<std::string> sql_check;
    void run() {
        sofi_demo_init();
        {
            sqlite::connection db{std::string{db_file}, false};
            for (auto&& sql: sql_prepare) {
                BOOST_TEST_INFO_SCOPE("sql_prepare: " << sql);
                auto op = [&] {
                    sqlite::query q{db, sql};
                    BOOST_TEST_REQUIRE(q.start().next_row() == sqlite::query::status::done);
                };
                BOOST_REQUIRE_NO_THROW(op());
            }
            sofi_demo_run();
            for (auto&& sql: sql_check) {
                BOOST_TEST_INFO_SCOPE("sql_check: " << sql);
                auto op = [&] {
                    sqlite::query q{db, sql};
                    BOOST_TEST_REQUIRE(q.start().next_row() == sqlite::query::status::row);
                    BOOST_TEST_REQUIRE(q.column_count() == 1);
                    auto v = q.get_column(0);
                    auto pb = std::get_if<int64_t>(&v);
                    BOOST_TEST_REQUIRE(pb);
                    BOOST_TEST(*pb == 1);
                    BOOST_TEST_REQUIRE(q.next_row() == sqlite::query::status::done);
                };
                BOOST_REQUIRE_NO_THROW(op());
            }
        }
    }
};

} // namespace

SOFICPP_IMPL_ENUM_STR_INIT(sqlite::query::status) {
    SOFICPP_IMPL_ENUM_STR_VAL(sqlite::query::status, row),
    SOFICPP_IMPL_ENUM_STR_VAL(sqlite::query::status, done),
    SOFICPP_IMPL_ENUM_STR_VAL(sqlite::query::status, locked),
};

namespace sqlite {

std::ostream& operator<<(std::ostream& os, sqlite::query::status v)
{
    os << soficpp::enum2str(v);
    return os;
}

} // namespace sqlite

//! \endcond

/*! \file
 * \test \c run -- Program \c sofi_demo can be executed without command line
 * arguments and exits with status 1. */
//! \cond
BOOST_AUTO_TEST_CASE(run)
{
    int status = system((sofi_demo_exe() // NOLINT(concurrency-mt-unsafe)
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

/*! \file
 * \test \c init -- Command `sofi_demo init` creates a SQLite database file. */
//! \cond
BOOST_AUTO_TEST_CASE(init)
{
    sofi_test{
        .sql_prepare = {
            R"(select * from request)", // table REQUEST is empty
            R"(select * from result)", // table RESULT is empty
        },
        .sql_check = {
            R"(select n == 0 from (select count() as n from request))",
            R"(select n == 0 from (select count() as n from result))",
        },
    }.run();
}
//! \endcond
