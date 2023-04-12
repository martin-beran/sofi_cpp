/*! \file
 * \brief Tests of classes unsed to implement concepts soficpp::message, soficpp::agent
 *
 * It tests declarations in file agent.hpp: classes soficpp::agent_result,
 * soficpp::copy_agent.
 */

//! \cond
#include "soficpp/agent.hpp"
#include "soficpp/soficpp.hpp"
#include <concepts>
#include <stdexcept>

#define BOOST_TEST_MODULE agent
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

namespace {

enum class op_id {
    test_no_flow,
    test_rd,
    test_wr,
    test_rd_wr,
};

} // namespace

SOFICPP_IMPL_ENUM_STR_INIT(op_id) {
    SOFICPP_IMPL_ENUM_STR_VAL(op_id, test_no_flow),
    SOFICPP_IMPL_ENUM_STR_VAL(op_id, test_rd),
    SOFICPP_IMPL_ENUM_STR_VAL(op_id, test_wr),
    SOFICPP_IMPL_ENUM_STR_VAL(op_id, test_rd_wr),
};

namespace {

[[maybe_unused]] std::ostream& operator<<(std::ostream& os, op_id v)
{
    os << soficpp::enum2str(v);
    return os;
}

using integrity = soficpp::integrity_set<std::string>;
using set_t = integrity::set_t;
using operation = soficpp::operation_base<op_id>;
using verdict = soficpp::simple_verdict;
using acl = soficpp::ops_acl<integrity, operation, verdict>;
using min_integrity = soficpp::acl_single<integrity, operation, verdict>;
using integrity_fun = soficpp::safe_integrity_fun<integrity, operation>;
using entity = soficpp::basic_entity<integrity, min_integrity, operation, verdict, acl, integrity_fun>;
using agent = soficpp::copy_agent<entity>;

class op_no_flow: public operation {
public:
    [[nodiscard]] id_t id() const override { return op_id::test_no_flow; }
    [[nodiscard]] std::string_view name() const override { return "op_no_flow"; }
};

class op_rd: public operation {
public:
    [[nodiscard]] bool is_read() const override { return true; }
    [[nodiscard]] id_t id() const override { return op_id::test_rd; }
    [[nodiscard]] std::string_view name() const override { return "op_rd"; }
};

class op_wr: public operation {
public:
    [[nodiscard]] bool is_write() const override { return true; }
    [[nodiscard]] id_t id() const override { return op_id::test_wr; }
    [[nodiscard]] std::string_view name() const override { return "op_wr"; }
};

class op_rd_wr: public operation {
public:
    [[nodiscard]] bool is_read() const override { return true; }
    [[nodiscard]] bool is_write() const override { return true; }
    [[nodiscard]] id_t id() const override { return op_id::test_rd_wr; }
    [[nodiscard]] std::string_view name() const override { return "op_rd_wr"; }
};

} // namespace
//! \endcond

/*! \file
 * \test \c copy_agent_export_success -- Test of function
 * soficpp::copy_agent::export_msg() with result
 * soficpp::agent_result::result::success */
//! \cond
BOOST_AUTO_TEST_CASE(copy_agent_export_success)
{
    agent a;
    entity e;
    e.integrity(integrity{set_t{"export_success"}});
    entity exported;
    auto r = a.export_msg(e, exported);
    BOOST_CHECK(bool(r));
    BOOST_CHECK(r.ok());
    BOOST_CHECK(r.code == soficpp::agent_result::success);
    BOOST_CHECK(exported.integrity() == e.integrity());
}
//! \endcond

/*! \file
 * \test \c copy_agent_export_error -- Test of function
 * soficpp::copy_agent::export_msg() with result
 * soficpp::agent_result::result::error */
//! \cond
BOOST_AUTO_TEST_CASE(copy_agent_export_error)
{
    agent a;
    a.export_result.code = soficpp::agent_result::error;
    entity e;
    e.integrity(integrity{set_t{"export_error"}});
    entity exported;
    auto r = a.export_msg(e, exported);
    BOOST_CHECK(!bool(r));
    BOOST_CHECK(!r.ok());
    BOOST_CHECK(r.code == soficpp::agent_result::error);
    BOOST_CHECK(exported.integrity() != e.integrity());
}
//! \endcond

/*! \file
 * \test \c copy_agent_export_untrusted -- Test of function
 * soficpp::copy_agent::export_msg() with result
 * soficpp::agent_result::result::untrusted */
//! \cond
BOOST_AUTO_TEST_CASE(copy_agent_export_untrusted)
{
    agent a;
    a.export_result.code = soficpp::agent_result::untrusted;
    entity e;
    e.integrity(integrity{set_t{"export_untrusted"}});
    entity exported;
    auto r = a.export_msg(e, exported);
    BOOST_CHECK(!bool(r));
    BOOST_CHECK(!r.ok());
    BOOST_CHECK(r.code == soficpp::agent_result::untrusted);
    BOOST_CHECK(exported.integrity() != e.integrity());
}
//! \endcond

/*! \file
 * \test \c copy_agent_import_success -- Test of function
 * soficpp::copy_agent::import_msg() with result
 * soficpp::agent_result::result::success */
//! \cond
BOOST_AUTO_TEST_CASE(copy_agent_import_success)
{
    agent a;
    entity e;
    e.integrity(integrity{set_t{"import_success"}});
    entity imported;
    auto r = a.import_msg(e, imported);
    BOOST_CHECK(bool(r));
    BOOST_CHECK(r.ok());
    BOOST_CHECK(r.code == soficpp::agent_result::success);
    BOOST_CHECK(imported.integrity() == e.integrity());
}
//! \endcond

/*! \file
 * \test \c copy_agent_import_error -- Test of function
 * soficpp::copy_agent::import_msg() with result
 * soficpp::agent_result::result::error */
//! \cond
BOOST_AUTO_TEST_CASE(copy_agent_import_error)
{
    agent a;
    a.import_result.code = soficpp::agent_result::error;
    entity e;
    e.integrity(integrity{set_t{"import_error"}});
    entity imported;
    auto r = a.import_msg(e, imported);
    BOOST_CHECK(!bool(r));
    BOOST_CHECK(!r.ok());
    BOOST_CHECK(r.code == soficpp::agent_result::error);
    BOOST_CHECK(imported.integrity() != e.integrity());
}
//! \endcond

/*! \file
 * \test \c copy_agent_import_untrusted -- Test of function
 * soficpp::copy_agent::import_msg() with result
 * soficpp::agent_result::result::untrusted */
//! \cond
BOOST_AUTO_TEST_CASE(copy_agent_import_untrusted)
{
    agent a;
    a.import_result.code = soficpp::agent_result::untrusted;
    entity e;
    e.integrity(integrity{set_t{"import_untrusted"}});
    entity imported;
    auto r = a.import_msg(e, imported);
    BOOST_CHECK(!bool(r));
    BOOST_CHECK(!r.ok());
    BOOST_CHECK(r.code == soficpp::agent_result::untrusted);
    BOOST_CHECK(imported.integrity() != e.integrity());
}
//! \endcond
