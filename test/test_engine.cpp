/*! \file
 * \brief Tests of class soficpp::engine in file engine.hpp
 */

//! \cond
#include "soficpp/entity.hpp"
#include "soficpp/soficpp.hpp"
#include <concepts>
#include <sstream>
#include <stdexcept>

#define BOOST_TEST_MODULE engine
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
using operation = soficpp::operation_base<op_id>;
using verdict = soficpp::simple_verdict;
using acl = soficpp::ops_acl<integrity, operation, verdict>;
using integrity_fun = soficpp::safe_integrity_fun<integrity, operation>;
using entity = soficpp::basic_entity<integrity, acl, operation, verdict, acl, integrity_fun>;

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

class engine: public soficpp::engine<entity> {
public:
    engine(bool str_stream = true) {
        if (str_stream)
            os = &str;
    }
    std::ostream* os = nullptr;
    std::ostringstream str;
protected:
    void init_verdict(const entity_t& subj, const entity_t& obj, const operation_t& op,
                      bool execute, verdict_t& v) override
    {
        soficpp::engine<entity>::init_verdict(subj, obj, op, execute, v);
        if (os)
            *os << "init_verdict execute=" << execute << std::endl;
    }
    void after_test_access(const entity_t& subj, const entity_t& obj, const operation_t& op,
                           bool execute, verdict_t& v, bool allow) override
    {
        soficpp::engine<entity>::after_test_access(subj, obj, op, execute, v, allow);
        if (os)
            *os << "after_test_access execute=" << execute << " allow=" << allow << std::endl;
    }
    void after_test_min(const entity_t& subj, const entity_t& obj, const operation_t& op, bool execute, verdict_t& v,
                        const std::optional<integrity_t>& i_subj, bool allow_min_subj,
                        const std::optional<integrity_t>& i_obj, bool allow_min_obj) override
    {
        soficpp::engine<entity>::after_test_min(subj, obj, op, execute, v,
                                                i_subj, allow_min_subj, i_obj, allow_min_obj);
        if (os)
            *os << "after_test_min execute=" << execute <<
                " i_subj=" << bool(i_subj) << " allow_min_subj=" << allow_min_subj <<
                " i_obj=" << bool(i_obj) << " allow_min_obj=" << allow_min_obj << std::endl;
    }
    void execute_op(entity_t& subj, entity_t& obj, const operation_t& op, verdict_t& v) override {
        soficpp::engine<entity>::execute_op(subj, obj, op, v);
        if (os)
            *os << "execute_op" << std::endl;
    }
};

} // namespace
//! \endcond

/*! \file
 * \test \c engine_exe_deny -- Operation execution denied by
 * soficpp::engine::operation() */
//! \cond
BOOST_AUTO_TEST_CASE(engine_exe_deny)
{
    engine e;
    entity subj;
    entity obj;
    verdict v = e.operation(subj, obj, op_no_flow());
    BOOST_CHECK(!v.allowed());
    BOOST_CHECK(!v.access_test());
    BOOST_CHECK(!v.min_test());
    BOOST_CHECK(e.str.str() == "init_verdict execute=1\nafter_test_access execute=1 allow=0\n");
}
//! \endcond

/*! \file
 * \test \c engine_notexe_deny -- Operation test-only denied by
 * soficpp::engine::operation() */
//! \cond
BOOST_AUTO_TEST_CASE(engine_notexe_deny)
{
    engine e;
    entity subj;
    entity obj;
    verdict v = e.operation(subj, obj, op_no_flow(), false);
    BOOST_CHECK(!v.allowed());
    BOOST_CHECK(!v.access_test());
    BOOST_CHECK(!v.min_test());
    BOOST_CHECK(e.str.str() == "init_verdict execute=0\nafter_test_access execute=0 allow=0\n");
}
//! \endcond

/*! \file
 * \test \c engine_exe_no_flow -- No-flow operation executed */
//! \cond
BOOST_AUTO_TEST_CASE(engine_exe_no_flow)
{
    engine e;
    entity subj;
    entity obj;
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_no_flow());
    BOOST_CHECK(v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(v.min_test());
    BOOST_CHECK(e.str.str() == "init_verdict execute=1\nafter_test_access execute=1 allow=1\n"
                "after_test_min execute=1 i_subj=0 allow_min_subj=1 i_obj=0 allow_min_obj=1\n"
                "execute_op\n");
}
//! \endcond

/*! \file
 * \test \c engine_notexe_no_flow -- No-flow test-only operation allowed */
//! \cond
BOOST_AUTO_TEST_CASE(engine_notexe_no_flow)
{
    engine e;
    entity subj;
    entity obj;
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_no_flow(), false);
    BOOST_CHECK(v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(v.min_test());
    BOOST_CHECK(e.str.str() == "init_verdict execute=0\nafter_test_access execute=0 allow=1\n"
                "after_test_min execute=0 i_subj=0 allow_min_subj=1 i_obj=0 allow_min_obj=1\n");
}
//! \endcond
