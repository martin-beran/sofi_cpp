/*! \file
 * \brief Tests of program \c sofi_demo
 *
 * Tests in this file check program \c sofi_demo and use this program for
 * testing various functionality of the SOFI C++ library. The tested program is
 * implemented in file sofi_demo.cpp.
 *
 * The tests create SQLite database file \c test_sofi_demo.db in the current
 * directory. If the file already exists, it is deleted first.
 *
 * Some tests create additional database tables. They are created as temporary
 * by default, accessible only by the database connection used for preparing
 * a tests and evaluating its results. If the program is started with custom
 * command line argument `-T`, that is, `test_sofi_demo -- -T`, these tables
 * are not temporary, so that can be examined from outside a test (for
 * debugging purposes).
 *
 * If the program is started with custom command line argument `-d`, SQL
 * commands used by class \c sofi_test are logged (by \c BOOST_TEST_MESSAGE).
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
#include <boost/test/data/test_case.hpp>

using namespace std::string_literals;
using namespace std::string_view_literals;

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

bool custom_arg(char a)
{
    auto&& mts = boost::unit_test::framework::master_test_suite();
    for (int i = 1; i < mts.argc; ++i)
        if (mts.argv[i][0] == '-' && mts.argv[i][1] == a)
            return true;
    return false;
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
    BOOST_TEST_MESSAGE(""); // adjusts color and reports context
    sofi_demo_do("run");
}

struct sql_grp {
    std::string name;
    std::vector<std::string> sql;
};

struct sofi_test {
    std::vector<sql_grp> sql_prepare;
    std::vector<sql_grp> sql_check;
    void run() {
        bool log_sql = custom_arg('d');
        sofi_demo_init();
        sqlite::connection db{std::string{db_file}, false};
        if (log_sql)
            for (auto&& grp: sql_prepare) {
                BOOST_TEST_MESSAGE("PREPARE GROUP " << grp.name);
                for (auto&& sql: grp.sql)
                    BOOST_TEST_MESSAGE("    " << sql << ";");
            }
        for (auto&& grp: sql_prepare) {
            BOOST_TEST_INFO_SCOPE("grp_prepare=" << grp.name);
            for (auto&& sql: grp.sql) {
                BOOST_TEST_INFO_SCOPE("sql_prepare: " << sql);
                auto op = [&] {
                    sqlite::query q{db, sql};
                    BOOST_TEST_REQUIRE(q.start().next_row() == sqlite::query::status::done);
                };
                BOOST_REQUIRE_NO_THROW(op());
            }
        }
        sofi_demo_run();
        if (log_sql)
            for (auto&& grp: sql_check) {
                BOOST_TEST_MESSAGE("CHECK GROUP " << grp.name);
                for (auto&& sql: grp.sql)
                    BOOST_TEST_MESSAGE("    " << sql << ";");
            }
        for (auto&& grp: sql_check) {
            BOOST_TEST_INFO_SCOPE("grp_check=" << grp.name);
            for (auto&& sql: grp.sql) {
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

// Functions usable for building SQL queries for sofi_test
namespace query {

std::string use_tmp_tbl()
{
    if (!custom_arg('T'))
        return " temporary ";
    else
        return " ";
}

sql_grp var()
{
    sql_grp grp{.name = "var", .sql = {
        R"(create)"s + use_tmp_tbl() + R"(table if not exists var (name text, value))"s,
        R"(insert into var select 'integrity_empty', id from integrity_json where elems = '[]' limit 1)"s,
        R"(insert into var select 'integrity_universe', id from integrity_json where elems = '"universe"' limit 1)"s,
        R"(insert into var select 'min_int_any', id from min_integrity_json2 where integrity = '[[]]' limit 1)"s,
        R"(insert into var select 'acl_allow', id from acl_json3 where acl = '{"":[[]]}' limit 1)"s,
        R"(insert into var select 'acl_deny', id from acl_json3 where acl = '{"":[]}' limit 1)"s,
        R"(insert into var select 'fun_min', id from int_fun_id where comment = 'min' limit 1)"s,
        R"(insert into var select 'fun_identity', id from int_fun_id where comment = 'identity' limit 1)"s,
        R"(insert into var select 'fun_max', id from int_fun_id where comment = 'max' limit 1)"s,
    }};
    return grp;
}

std::string var(const std::string& name)
{
    return R"((select value from var where name=')"s + name + "')";
}

std::string var(const std::string& name, const std::string& value_sql)
{
    return R"(insert into var values (')" + name + R"(', ()" + value_sql + R"()))";
}

} // namespace query

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
        .sql_prepare = {{ "", {
            R"(select * from request)", // table REQUEST is empty
            R"(select * from result)", // table RESULT is empty
        }}},
        .sql_check = {{ "", {
            R"(select n == 0 from (select count() as n from request))",
            R"(select n == 0 from (select count() as n from result))",
        }}},
    }.run();
}

namespace {

struct test_op {
    std::string before_subj;
    std::string before_obj;
    std::string op;
    std::string arg;
    std::string after_subj;
    std::string after_obj;
};

std::ostream& operator<<(std::ostream& os, const test_op& v)
{
    os << "{subj=\"" << v.before_subj << "\",obj=\"" << v.before_obj << "\",op=" << v.op <<
        ",arg=\"" << v.arg << "\",subj=\"" << v.after_subj << "\",obj=\"" << v.after_obj << "\"}";
    return os;
}

} // namespace
//! \endcond

/*! \file
 * \test \c op -- Execution of operations */
//! \cond
BOOST_DATA_TEST_CASE(op, (std::array{
    test_op{"[subj_data]", "[obj_data]", "append_arg", "[argument]", "[subj_data]", "[obj_data][argument]"},
    test_op{"[subj_data]", "[obj_data]", "no_op", "[argument]", "[subj_data]", "[obj_data]"},
    test_op{"[subj_data]", "[obj_data]", "read", "[argument]", "[obj_data]", "[obj_data]"},
    test_op{"[subj_data]", "[obj_data]", "read_append", "[argument]", "[subj_data][obj_data]", "[obj_data]"},
    test_op{"[subj_data]", "[obj_data]", "swap", "[argument]", "[obj_data]", "[subj_data]"},
    test_op{"[subj_data]", "[obj_data]", "write", "[argument]", "[subj_data]", "[subj_data]"},
    test_op{"[subj_data]", "[obj_data]", "write_append", "[argument]", "[subj_data]", "[obj_data][subj_data]"},
}))
{
    sofi_test{
        .sql_prepare = {
            query::var(),
            { "entities", {
                R"(insert into entity values ('subject', )"s +
                    query::var("integrity_universe") + R"(, )"s + query::var("min_int_any") + R"(, )" +
                    query::var("acl_allow") + R"(, )" + query::var("fun_identity") + R"(, )" +
                    query::var("fun_identity") + R"(, )" + query::var("fun_identity") +
                    R"(, ')" + sample.before_subj + R"('))",
                R"(insert into entity values ('object', )"s +
                    query::var("integrity_universe") + R"(, )"s + query::var("min_int_any") + R"(, )" +
                    query::var("acl_allow") + R"(, )" + query::var("fun_identity") + R"(, )" +
                    query::var("fun_identity") + R"(, )" + query::var("fun_identity") +
                    R"(, ')" + sample.before_obj + R"('))",
            }},
            { "requests", {
                R"(insert into request_ins values
                    ('subject', 'object', ')" + sample.op + R"(', ')" + sample.arg + R"(', ''))",
            }},
        },
        .sql_check = {
            { "op_count", {
                R"(select count() == 0 from request)",
                R"(select count() == 1 from result)",
            }},
            { "op_result", {
                R"(select
                        subject == 'subject' and object == 'object' and op == ')" + sample.op + R"(' and
                        allowed and access and min and not error
                    from result where id == 0)",
                R"(select data == ')" + sample.after_subj + R"(' from entity where name = 'subject')",
                R"(select data == ')" + sample.after_obj + R"(' from entity where name = 'object')",
            }},
        },
    }.run();
}
//! \endcond

/*! \file
 * \test \c op_clone -- Execution of operation \c clone */
//! \cond
BOOST_AUTO_TEST_CASE(op_clone)
{
    sofi_test{
        .sql_prepare = {
            query::var(),
            { "entities", {
                R"(insert into entity values ('subject', )"s +
                    query::var("integrity_universe") + R"(, )"s + query::var("min_int_any") + R"(, )" +
                    query::var("acl_allow") + R"(, )" + query::var("fun_identity") + R"(, )" +
                    query::var("fun_identity") + R"(, )" + query::var("fun_identity") + R"(, '[subj_data]'))",
                R"(insert into entity values ('object', )"s +
                    query::var("integrity_universe") + R"(, )"s + query::var("min_int_any") + R"(, )" +
                    query::var("acl_allow") + R"(, )" + query::var("fun_identity") + R"(, )" +
                    query::var("fun_identity") + R"(, )" + query::var("fun_identity") + R"(, '[obj_data]'))",
            }},
            { "requests", {
                R"(insert into request_ins values ('subject', 'object', 'clone', 'copy_of_object', ''))",
            }},
        },
        .sql_check = {
            { "op_result", {
                R"(select count() == 3 from entity)",
                R"(select data == '[subj_data]' from entity where name == 'subject')",
                R"(select data == '[obj_data]' from entity where name == 'object')",
                R"(select data == '[obj_data]' from entity where name == 'copy_of_object')",
            }},
        },
    }.run();
}
//! \endcond

/*! \file
 * \test \c op_destroy -- Execution of operation \c destroy */
//! \cond
BOOST_AUTO_TEST_CASE(op_destroy)
{
    sofi_test{
        .sql_prepare = {
            query::var(),
            { "entities", {
                R"(insert into entity values ('subject', )"s +
                    query::var("integrity_universe") + R"(, )"s + query::var("min_int_any") + R"(, )" +
                    query::var("acl_allow") + R"(, )" + query::var("fun_identity") + R"(, )" +
                    query::var("fun_identity") + R"(, )" + query::var("fun_identity") + R"(, '[subj_data]'))",
                R"(insert into entity values ('object', )"s +
                    query::var("integrity_universe") + R"(, )"s + query::var("min_int_any") + R"(, )" +
                    query::var("acl_allow") + R"(, )" + query::var("fun_identity") + R"(, )" +
                    query::var("fun_identity") + R"(, )" + query::var("fun_identity") + R"(, '[obj_data]'))",
            }},
            { "requests", {
                R"(insert into request_ins values ('subject', 'object', 'destroy', '', ''))",
            }},
        },
        .sql_check = {
            { "op_result", {
                R"(select count() == 1 from entity)",
                R"(select data == '[subj_data]' from entity where name == 'subject')",
                R"(select count() == 0 from entity where name == 'object')",
            }},
        },
    }.run();
}
//! \endcond

/*! \file
 * \test \c op_set_integrity -- Execution of operation \c set_integrity */
//! \cond
BOOST_AUTO_TEST_CASE(op_set_integrity)
{
    sofi_test{
        .sql_prepare = {
            query::var(),
            { "entities", {
                R"(insert into entity values ('subject', )"s +
                    query::var("integrity_universe") + R"(, )"s + query::var("min_int_any") + R"(, )" +
                    query::var("acl_allow") + R"(, )" + query::var("fun_identity") + R"(, )" +
                    query::var("fun_identity") + R"(, )" + query::var("fun_identity") + R"(, '[subj_data]'))",
                R"(insert into entity values ('object', )"s +
                    query::var("integrity_universe") + R"(, )"s + query::var("min_int_any") + R"(, )" +
                    query::var("acl_allow") + R"(, )" + query::var("fun_identity") + R"(, )" +
                    query::var("fun_identity") + R"(, )" + query::var("fun_identity") + R"(, '[obj_data]'))",
            }},
            { "requests", {
                R"(insert into request_ins values ('subject', 'object', 'set_integrity', '["changed_integrity"]', ''))",
            }},
        },
        .sql_check = {
            { "op_result", {
                R"(select count() == 2 from entity)",
                R"(select data == '[subj_data]' from entity where name == 'subject')",
                R"(select data == '[obj_data]' from entity where name == 'object')",
                R"(select elems == '"universe"'
                    from entity join integrity_json on entity.integrity == integrity_json.id where name=='subject')",
                R"(select min_integrity_json.integrity == '[]'
                    from entity join min_integrity_json on entity.min_integrity == min_integrity_json.id
                    where name=='subject')",
                R"(select elems == '["changed_integrity"]'
                    from entity join integrity_json on entity.integrity == integrity_json.id where name=='object')",
                R"(select min_integrity_json.integrity == '[]'
                    from entity join min_integrity_json on entity.min_integrity == min_integrity_json.id
                    where name=='object')",
            }},
        },
    }.run();
}
//! \endcond

/*! \file
 * \test \c op_set_min_integrity -- Execution of operation \c set_min_integrity */
//! \cond
BOOST_AUTO_TEST_CASE(op_set_min_integrity)
{
    sofi_test{
        .sql_prepare = {
            query::var(),
            { "entities", {
                R"(insert into entity values ('subject', )"s +
                    query::var("integrity_universe") + R"(, )"s + query::var("min_int_any") + R"(, )" +
                    query::var("acl_allow") + R"(, )" + query::var("fun_identity") + R"(, )" +
                    query::var("fun_identity") + R"(, )" + query::var("fun_identity") + R"(, '[subj_data]'))",
                R"(insert into entity values ('object', )"s +
                    query::var("integrity_universe") + R"(, )"s + query::var("min_int_any") + R"(, )" +
                    query::var("acl_allow") + R"(, )" + query::var("fun_identity") + R"(, )" +
                    query::var("fun_identity") + R"(, )" + query::var("fun_identity") + R"(, '[obj_data]'))",
            }},
            { "requests", {
                R"(insert into request_ins values
                    ('subject', 'object', 'set_min_integrity', '[["min1"],["min2"]]', ''))",
            }},
        },
        .sql_check = {
            { "op_result", {
                R"(select count() == 2 from entity)",
                R"(select data == '[subj_data]' from entity where name == 'subject')",
                R"(select data == '[obj_data]' from entity where name == 'object')",
                R"(select elems == '"universe"'
                    from entity join integrity_json on entity.integrity == integrity_json.id where name=='subject')",
                R"(select min_integrity_json.integrity == '[]'
                    from entity join min_integrity_json on entity.min_integrity == min_integrity_json.id
                    where name=='subject')",
                R"(select elems == '"universe"'
                    from entity join integrity_json on entity.integrity == integrity_json.id where name=='object')",
                R"(with
                        mi(i) as (
                            select min_integrity_json.integrity
                            from entity join min_integrity_json on entity.min_integrity == min_integrity_json.id
                            where name=='object'
                        ),
                        exp(i) as (values ('["min1"]'), ('["min2"]'))
                    select (select count() from mi) == (select count() from exp) and
                        (select count() from mi join exp using (i)) == (select count() from exp))",
            }},
        },
    }.run();
}

namespace {

struct test_op_acl {
    std::string subj_int1;
    std::string subj_min1;
    std::string obj_int1;
    std::string obj_min1;
    std::string acl;
    std::string op;
    bool allowed;
    bool access;
    bool min;
    std::string subj_int2;
    std::string obj_int2;
};

std::ostream& operator<<(std::ostream& os, const test_op_acl& v)
{
    os << "{subj=" << v.subj_int1 << "/" << v.subj_min1 << " obj=" << v.obj_int1 << "/" << v.obj_min1 <<
        " -> " << v.op << "/" << v.acl << " -> allowed=" << v.allowed << " access=" << v.access << " min=" << v.min <<
        " subj=" << v.subj_int2 << " obj=" << v.obj_int2 << "}";
    return os;
}

void run_test(const test_op_acl& sample)
{
    auto var_int = [](auto&& name, auto&& value) {
        return sql_grp{.name = name, .sql = {
            R"(insert into integrity_json values (null, ')" + value + R"('))",
            query::var(name, R"(select id from integrity_id_max)"),
        }};
    };
    auto var_min = [](auto&& name, auto&& value) {
        return sql_grp{.name = name, .sql = {
            R"(insert into acl_json2 values (null, null, ')" + value + R"('))",
            query::var(name, R"(select id from acl_id_max)"),
        }};
    };
    sofi_test{
        .sql_prepare = {
            query::var(),
            var_int("subj_int1", sample.subj_int1),
            var_min("subj_min1", sample.subj_min1),
            var_int("obj_int1", sample.obj_int1),
            var_min("obj_min1", sample.obj_min1),
            { "acls", {
                R"(insert into acl_json3 values (null, ')" + sample.acl + R"('))",
                query::var("acl", R"(select id from acl_id_max)"),
            }},
            { "entities", {
                R"(insert into entity values ('subject', )"s +
                    query::var("subj_int1") + R"(, )"s + query::var("subj_min1") + R"(, )"s +
                    query::var("acl_deny") + R"(, )"s + query::var("fun_identity") + R"(, )"s +
                    query::var("fun_min") + R"(, )"s + query::var("fun_max") + R"(, 'subj_data'))"s,
                R"(insert into entity values ('object', )"s +
                    query::var("obj_int1") + R"(, )"s + query::var("obj_min1") + R"(, )"s +
                    query::var("acl") + R"(, )"s + query::var("fun_identity") + R"(, )"s +
                    query::var("fun_min") + R"(, )"s + query::var("fun_max") + R"(, 'obj_data'))"s,
            }},
            { "requests", {
                R"(insert into request_ins values
                    ('subject', 'object', ')"s + sample.op + R"(', 'arg', ''))"s,
            }},
        },
        .sql_check = {
            { "op_result", {
                R"(select
                        subject == 'subject' and object == 'object' and op == ')"s + sample.op + R"(' and
                        )" + (sample.allowed ? "" : "not ") + R"(allowed and
                        )" + (sample.access ? "" : "not ") + R"(access and
                        )" + (sample.min ? "" : "not ") + R"(min and not error
                    from result where id == 0)",
                R"(select i.elems == ')"s + sample.subj_int2 + R"('
                    from entity as e join integrity_json as i on e.integrity == i.id
                    where e.name == 'subject')"s,
                R"(select i.elems == ')"s + sample.obj_int2 + R"('
                    from entity as e join integrity_json as i on e.integrity == i.id
                    where e.name == 'object')"s,
            }},
        },
    }.run();
}

} // namespace
//! \endcond

/*! \file
 * \test \c op_allowed -- Operation allowed */
//! \cond
BOOST_DATA_TEST_CASE(op_allowed, (std::array{
    // no-flow, universe integrity, ACL allows all
    test_op_acl{.subj_int1 = R"("universe")", .subj_min1 = R"([[]])",
        .obj_int1 = R"("universe")", .obj_min1 = R"([[]])", .acl = R"({"":[[]]})",
        .op = R"(no_op)", .allowed = true, .access = true, .min = true,
        .subj_int2 = R"("universe")", .obj_int2 = R"("universe")"},
    // read, universe integrity, ACL allows all
    test_op_acl{.subj_int1 = R"("universe")", .subj_min1 = R"([[]])",
        .obj_int1 = R"("universe")", .obj_min1 = R"([[]])", .acl = R"({"":[[]]})",
        .op = R"(read)", .allowed = true, .access = true, .min = true,
        .subj_int2 = R"("universe")", .obj_int2 = R"("universe")"},
    // write, universe integrity, ACL allows all
    test_op_acl{.subj_int1 = R"("universe")", .subj_min1 = R"([[]])",
        .obj_int1 = R"("universe")", .obj_min1 = R"([[]])", .acl = R"({"":[[]]})",
        .op = R"(write)", .allowed = true, .access = true, .min = true,
        .subj_int2 = R"("universe")", .obj_int2 = R"("universe")"},
    // read-write, universe integrity, ACL allows all
    test_op_acl{.subj_int1 = R"("universe")", .subj_min1 = R"([[]])",
        .obj_int1 = R"("universe")", .obj_min1 = R"([[]])", .acl = R"({"":[[]]})",
        .op = R"(swap)", .allowed = true, .access = true, .min = true,
        .subj_int2 = R"("universe")", .obj_int2 = R"("universe")"},
    // TODO
}))
{
    run_test(sample);
}
//! \endcond

/*! \file
 * \test \c op_denied_access -- Operation denied by access test */
//! \cond
BOOST_DATA_TEST_CASE(op_denied_access, (std::array{
    // minimum integrity, ACL requires greater than minimum
    test_op_acl{.subj_int1 = R"([])", .subj_min1 = R"([[]])",
        .obj_int1 = R"("universe")", .obj_min1 = R"([[]])", .acl = R"({"":[["i1"]]})",
        .op = R"(no_op)", .allowed = false, .access = false, .min = false,
        .subj_int2 = R"([])", .obj_int2 = R"("universe")"},
    // TODO
}))
{
    run_test(sample);
}
//! \endcond

/*! \file
 * \test \c op_denied_min -- Operation denied by minimum integrity test */
//! \cond
BOOST_DATA_TEST_CASE(op_denied_min, (std::array{
    // read, ACL allows all
    test_op_acl{.subj_int1 = R"(["subj"])", .subj_min1 = R"([["subj"]])",
        .obj_int1 = R"(["obj"])", .obj_min1 = R"([["obj"]])", .acl = R"({"":[[]]})",
        .op = R"(read)", .allowed = false, .access = true, .min = false,
        .subj_int2 = R"(["subj"])", .obj_int2 = R"(["obj"])"},
    // write, ACL allows all
    test_op_acl{.subj_int1 = R"(["subj"])", .subj_min1 = R"([["subj"]])",
        .obj_int1 = R"(["obj"])", .obj_min1 = R"([["obj"]])", .acl = R"({"":[[]]})",
        .op = R"(write)", .allowed = false, .access = true, .min = false,
        .subj_int2 = R"(["subj"])", .obj_int2 = R"(["obj"])"},
    // read-write, ACL allows all
    test_op_acl{.subj_int1 = R"(["subj"])", .subj_min1 = R"([["subj"]])",
        .obj_int1 = R"(["obj"])", .obj_min1 = R"([["obj"]])", .acl = R"({"":[[]]})",
        .op = R"(swap)", .allowed = false, .access = true, .min = false,
        .subj_int2 = R"(["subj"])", .obj_int2 = R"(["obj"])"},
    // TODO
}))
{
    run_test(sample);
}
//! \endcond
