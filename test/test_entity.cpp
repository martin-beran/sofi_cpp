/*! \file
 * \brief Tests of classes used to implement concept soficpp::entity
 *
 * It tests declarations in file entity.hpp: classes soficpp::operation_base,
 * soficpp::simple_verdict, soficpp::acl_single, soficpp::acl,
 * soficpp::ops_acl, soficpp::dyn_integrity_fun, soficpp::integrity_fun,
 * soficpp::safe_integrity_fun, soficpp::basic_entity
 */

//! \cond
#include "soficpp/entity.hpp"
#include "soficpp/soficpp.hpp"
#include <concepts>
#include <stdexcept>

#define BOOST_TEST_MODULE entity
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

std::ostream& operator<<(std::ostream& os, op_id v)
{
    os << soficpp::enum2str(v);
    return os;
}

class op_no_flow: public soficpp::operation_base<op_id> {
public:
    [[nodiscard]] id_t id() const override { return op_id::test_no_flow; }
    [[nodiscard]] std::string_view name() const override { return "op_no_flow"; }
};

class op_rd: public soficpp::operation_base<op_id> {
public:
    [[nodiscard]] bool is_read() const override { return true; }
    [[nodiscard]] id_t id() const override { return op_id::test_rd; }
    [[nodiscard]] std::string_view name() const override { return "op_rd"; }
};

class op_wr: public soficpp::operation_base<op_id> {
public:
    [[nodiscard]] bool is_write() const override { return true; }
    [[nodiscard]] id_t id() const override { return op_id::test_wr; }
    [[nodiscard]] std::string_view name() const override { return "op_wr"; }
};

class op_rd_wr: public soficpp::operation_base<op_id> {
public:
    [[nodiscard]] bool is_read() const override { return true; }
    [[nodiscard]] bool is_write() const override { return true; }
    [[nodiscard]] id_t id() const override { return op_id::test_rd_wr; }
    [[nodiscard]] std::string_view name() const override { return "op_rd_wr"; }
};

} // namespace
//! \endcond

/*! \file
 * \test \c operation_base -- Test of class soficpp::operation_base */
//! \cond
BOOST_AUTO_TEST_CASE(operation_base)
{
    {
        std::unique_ptr<const soficpp::operation_base<op_id>> op = std::make_unique<soficpp::operation_base<op_id>>();
        BOOST_CHECK(!op->is_read());
        BOOST_CHECK(!op->is_write());
        BOOST_CHECK(op->key() == op_id::test_no_flow);
        BOOST_CHECK(op->id() == op_id::test_no_flow);
        BOOST_CHECK(op->name().empty());
    }
    {
        std::unique_ptr<const soficpp::operation_base<op_id>> op = std::make_unique<op_no_flow>();
        BOOST_CHECK(!op->is_read());
        BOOST_CHECK(!op->is_write());
        BOOST_CHECK(op->key() == op_id::test_no_flow);
        BOOST_CHECK(op->id() == op_id::test_no_flow);
        BOOST_CHECK(op->name() == "op_no_flow");
    }
    {
        std::unique_ptr<const soficpp::operation_base<op_id>> op = std::make_unique<op_rd>();
        BOOST_CHECK(op->is_read());
        BOOST_CHECK(!op->is_write());
        BOOST_CHECK(op->key() == op_id::test_rd);
        BOOST_CHECK(op->id() == op_id::test_rd);
        BOOST_CHECK(op->name() == "op_rd");
    }
    {
        std::unique_ptr<const soficpp::operation_base<op_id>> op = std::make_unique<op_wr>();
        BOOST_CHECK(!op->is_read());
        BOOST_CHECK(op->is_write());
        BOOST_CHECK(op->key() == op_id::test_wr);
        BOOST_CHECK(op->id() == op_id::test_wr);
        BOOST_CHECK(op->name() == "op_wr");
    }
    {
        std::unique_ptr<const soficpp::operation_base<op_id>> op = std::make_unique<op_rd_wr>();
        BOOST_CHECK(op->is_read());
        BOOST_CHECK(op->is_write());
        BOOST_CHECK(op->key() == op_id::test_rd_wr);
        BOOST_CHECK(op->id() == op_id::test_rd_wr);
        BOOST_CHECK(op->name() == "op_rd_wr");
    }
}

namespace {

struct verdict_values {
    bool access;
    bool min;
};

std::ostream& operator<<(std::ostream& os, const verdict_values& v)
{
    os << "{.access=" << v.access << ",.min=" << v.min << "}";
    return os;
}

} // namespace
//! \endcond

/*! \file
 * \test \c simple_verdict -- Test of class soficpp::simple_verdict */
//! \cond
BOOST_DATA_TEST_CASE(simple_verdict, (std::array{
    verdict_values{ .access = false, .min = false, },
    verdict_values{ .access = false, .min = true, }, // cannot be set by soficpp::engine::operation()
    verdict_values{ .access = true, .min = false, },
    verdict_values{ .access = true, .min = true, },
}))
{
    soficpp::simple_verdict v;
    BOOST_CHECK(!bool(v));
    BOOST_CHECK(!v.allowed());
    BOOST_CHECK(!v.access_test());
    BOOST_CHECK(!v.min_test());
    v.access_test(sample.access);
    BOOST_CHECK(!bool(v));
    BOOST_CHECK(!v.allowed());
    BOOST_CHECK(v.access_test() == sample.access);
    BOOST_CHECK(!v.min_test());
    v.min_test(sample.min);
    BOOST_CHECK(bool(v) == (sample.access && sample.min));
    BOOST_CHECK(v.allowed() == (sample.access && sample.min));
    BOOST_CHECK(v.access_test() == sample.access);
    BOOST_CHECK(v.min_test() == sample.min);
}

namespace {

using integrity = soficpp::integrity_set<std::string>;
using set_t = integrity::set_t;
using universe = integrity::universe;
using operation = soficpp::operation_base<op_id>;
using verdict = soficpp::simple_verdict;

using acl_single_t = soficpp::acl_single<integrity, operation, verdict>;
using acl_t = soficpp::acl<integrity, operation, verdict>;
using ops_acl_t = soficpp::ops_acl<integrity, operation, verdict>;

struct acl_single_data {
    integrity i_acl;
    integrity i_subj;
    bool test;
};

struct acl_data {
    std::vector<integrity> i_acl;
    integrity i_subj;
    bool test;
};

struct ops_acl_data {
    std::map<operation::key_t, std::vector<integrity>> i_acl;
    std::vector<integrity> i_default;
    integrity i_subj;
    op_id op;
    bool test;
};

std::ostream& operator<<(std::ostream& os, const acl_single_data& v)
{
    os << "{i_acl=" << v.i_acl << ",i_subj=" << v.i_subj << ",test=" << v.test << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const acl_data& v)
{
    os << "{i_acl={";
    for (std::string_view d{}; auto&& e: v.i_acl) {
        os << d << e;
        d = ",";
    }
    os << "},i_subj=" << v.i_subj << ",test=" << v.test << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const ops_acl_data& v)
{
    os << "{i_acl={";
    for (std::string_view md{}; auto&& me: v.i_acl) {
        os << md << me.first << "->{";
        for (std::string_view vd{}; auto&& ve: me.second) {
            os << vd << ve;
            vd = ",";
        }
        os << "}";
        md = ",";
    }
    os << "},i_default={";
    for (std::string_view d{}; auto&& e: v.i_default) {
        os << d << e;
        d = ",";
    }
    os << "},i_subj=" << v.i_subj << ",op=" << v.op << ",test=" << v.test << "}";
    return os;
}

} // namespace
//! \endcond

/*! \file
 * \test \c acl_single -- Test of class soficpp::acl_single */
//! \cond
BOOST_DATA_TEST_CASE(acl_single, (std::array{
    acl_single_data{.i_acl = {}, .i_subj = integrity{}, .test = true, },
    acl_single_data{.i_acl = {}, .i_subj = integrity{set_t{"i1"}}, .test = true, },
    acl_single_data{.i_acl = {}, .i_subj = integrity{universe{}}, .test = true, },
    acl_single_data{.i_acl = integrity{set_t{"i1", "i2"}}, .i_subj = {}, .test = false, },
    acl_single_data{.i_acl = integrity{set_t{"i1", "i2"}}, .i_subj = integrity{set_t{"i2"}}, .test = false, },
    acl_single_data{.i_acl = integrity{set_t{"i1", "i2"}}, .i_subj = integrity{set_t{"i1", "i2"}}, .test = true, },
    acl_single_data{.i_acl = integrity{set_t{"i1", "i2"}},
        .i_subj = integrity{set_t{"i1", "i2", "i3"}}, .test = true, },
    acl_single_data{.i_acl = integrity{set_t{"i1", "i2"}}, .i_subj = integrity{universe{}}, .test = true, },
    acl_single_data{.i_acl = integrity{universe{}}, .i_subj = integrity{}, .test = false, },
    acl_single_data{.i_acl = integrity{universe{}}, .i_subj = integrity{set_t{"i1", "i2"}}, .test = false, },
    acl_single_data{.i_acl = integrity{universe{}}, .i_subj = integrity{universe{}}, .test = true, },
}))
{
    acl_single_t acl{sample.i_acl};
    op_no_flow op{};
    verdict v;
    for (auto k:
         {soficpp::controller_test::access, soficpp::controller_test::min_subj, soficpp::controller_test::min_obj})
    {
        BOOST_CHECK(acl.test(sample.i_subj, op, v, k) == sample.test);
    }
}
//! \endcond

/*! \file
 * \test \c acl -- Test of class soficpp::acl */
//! \cond
BOOST_DATA_TEST_CASE(acl, (std::array{
    acl_data{.i_acl = {}, .i_subj = integrity{}, .test = false, },
    acl_data{.i_acl = {}, .i_subj = integrity{set_t{"i1"}}, .test = false, },
    acl_data{.i_acl = {}, .i_subj = integrity{universe{}}, .test = false, },
    acl_data{.i_acl = {integrity{}}, .i_subj = integrity{}, .test = true, },
    acl_data{.i_acl = {integrity{}}, .i_subj = integrity{set_t{"i1"}}, .test = true, },
    acl_data{.i_acl = {integrity{}}, .i_subj = integrity{universe{}}, .test = true, },
    acl_data{.i_acl = {integrity{universe{}}, integrity{}, integrity{set_t{"i1"}}},
        .i_subj = integrity{}, .test = true, },
    acl_data{.i_acl = {integrity{universe{}}, integrity{}, integrity{set_t{"i1"}}},
        .i_subj = integrity{set_t{"i1"}}, .test = true, },
    acl_data{.i_acl = {integrity{universe{}}, integrity{}, integrity{set_t{"i1"}}},
        .i_subj = integrity{universe{}}, .test = true, },
    acl_data{.i_acl = {integrity{universe{}}}, .i_subj = integrity{}, .test = false, },
    acl_data{.i_acl = {integrity{universe{}}}, .i_subj = integrity{set_t{"i1", "i2", "i3"}}, .test = false, },
    acl_data{.i_acl = {integrity{universe{}}}, .i_subj = integrity{universe{}}, .test = true, },
    acl_data{.i_acl = {integrity{set_t{"i1"}}, integrity{set_t{"i2", "i3"}}},
        .i_subj = integrity{}, .test = false, },
    acl_data{.i_acl = {integrity{set_t{"i1"}}, integrity{set_t{"i2", "i3"}}},
        .i_subj = integrity{set_t{"i1"}}, .test = true, },
    acl_data{.i_acl = {integrity{set_t{"i1"}}, integrity{set_t{"i2", "i3"}}},
        .i_subj = integrity{set_t{"i1", "i2", "i3"}}, .test = true, },
    acl_data{.i_acl = {integrity{set_t{"i1"}}, integrity{set_t{"i2", "i3"}}},
        .i_subj = integrity{set_t{"i1", "i4"}}, .test = true, },
    acl_data{.i_acl = {integrity{set_t{"i1"}}, integrity{set_t{"i2", "i3"}}},
        .i_subj = integrity{set_t{"i2", "i3"}}, .test = true, },
    acl_data{.i_acl = {integrity{set_t{"i1"}}, integrity{set_t{"i2", "i3"}}},
        .i_subj = integrity{set_t{"i2", "i3", "i4"}}, .test = true, },
    acl_data{.i_acl = {integrity{set_t{"i1"}}, integrity{set_t{"i2", "i3"}}},
        .i_subj = integrity{set_t{"i3", "i4"}}, .test = false, },
    acl_data{.i_acl = {integrity{set_t{"i1"}}, integrity{set_t{"i2", "i3"}}},
        .i_subj = integrity{set_t{"i4", "i5"}}, .test = false, },
    acl_data{.i_acl = {integrity{set_t{"i1"}}, integrity{set_t{"i2", "i3"}}},
        .i_subj = integrity{universe{}}, .test = true, },
}))
{
    acl_t acl{sample.i_acl};
    op_no_flow op{};
    verdict v;
    for (auto k:
         {soficpp::controller_test::access, soficpp::controller_test::min_subj, soficpp::controller_test::min_obj})
    {
        BOOST_CHECK(acl.test(sample.i_subj, op, v, k) == sample.test);
    }
}
//! \endcond

/*! \file
 * \test \c ops_acl -- Test of class soficpp::ops_acl */
//! \cond
BOOST_DATA_TEST_CASE(ops_acl, (std::array{
    ops_acl_data{.i_acl = {},
        .i_default = {}, .i_subj = integrity{}, .op = op_id::test_no_flow, .test = false},
    ops_acl_data{.i_acl = {},
        .i_default = {integrity{}}, .i_subj = integrity{}, .op = op_id::test_rd, .test = true},
    ops_acl_data{.i_acl = {},
        .i_default = {integrity{universe{}}}, .i_subj = integrity{}, .op = op_id::test_wr, .test = false},
    ops_acl_data{.i_acl = {},
        .i_default = {integrity{set_t{"i1"}}}, .i_subj = integrity{set_t{"i1", "i2"}}, .test = true},
    ops_acl_data{.i_acl = {{op_id::test_no_flow, {}}},
        .i_default = {integrity{}}, .i_subj = integrity{universe{}}, .op = op_id::test_no_flow, .test = false},
    ops_acl_data{.i_acl = {{op_id::test_no_flow, {}}},
        .i_default = {integrity{}}, .i_subj = integrity{universe{}}, .op = op_id::test_rd_wr, .test = true},
    ops_acl_data{.i_acl = {{op_id::test_rd, {integrity{set_t{"i1"}}}}, {op_id::test_wr, {integrity{set_t{"i2"}}}}},
        .i_default = {integrity{set_t{"i3"}}},
        .i_subj = integrity{set_t{"i1", "i3"}}, .op = op_id::test_no_flow, .test = true},
    ops_acl_data{.i_acl = {{op_id::test_rd, {integrity{set_t{"i1"}}}}, {op_id::test_wr, {integrity{set_t{"i2"}}}}},
        .i_default = {integrity{set_t{"i3"}}},
        .i_subj = integrity{set_t{"i2"}}, .op = op_id::test_no_flow, .test = false},
    ops_acl_data{.i_acl = {{op_id::test_rd, {integrity{set_t{"i1"}}}}, {op_id::test_wr, {integrity{set_t{"i2"}}}}},
        .i_default = {integrity{set_t{"i3"}}},
        .i_subj = integrity{set_t{"i1", "i3"}}, .op = op_id::test_rd, .test = true},
    ops_acl_data{.i_acl = {{op_id::test_rd, {integrity{set_t{"i1"}}}}, {op_id::test_wr, {integrity{set_t{"i2"}}}}},
        .i_default = {integrity{set_t{"i3"}}},
        .i_subj = integrity{set_t{"i2"}}, .op = op_id::test_rd, .test = false},
    ops_acl_data{.i_acl = {{op_id::test_rd, {integrity{set_t{"i1"}}}}, {op_id::test_wr, {integrity{set_t{"i2"}}}}},
        .i_default = {integrity{set_t{"i3"}}},
        .i_subj = integrity{set_t{"i1", "i3"}}, .op = op_id::test_wr, .test = false},
    ops_acl_data{.i_acl = {{op_id::test_rd, {integrity{set_t{"i1"}}}}, {op_id::test_wr, {integrity{set_t{"i2"}}}}},
        .i_default = {integrity{set_t{"i3"}}},
        .i_subj = integrity{set_t{"i2"}}, .op = op_id::test_wr, .test = true},
    ops_acl_data{.i_acl = {{op_id::test_rd, {integrity{set_t{"i1"}}}}, {op_id::test_wr, {integrity{set_t{"i2"}}}}},
        .i_default = {integrity{set_t{"i3"}}},
        .i_subj = integrity{set_t{"i1", "i3"}}, .op = op_id::test_rd_wr, .test = true},
    ops_acl_data{.i_acl = {{op_id::test_rd, {integrity{set_t{"i1"}}}}, {op_id::test_wr, {integrity{set_t{"i2"}}}}},
        .i_default = {integrity{set_t{"i3"}}},
        .i_subj = integrity{set_t{"i2"}}, .op = op_id::test_rd_wr, .test = false},
}))
{
    ops_acl_t acl{sample.i_default};
    for (auto&& a: sample.i_acl)
        acl[a.first] = std::make_shared<typename ops_acl_t::acl_t>(a.second);
    std::unique_ptr<const soficpp::operation_base<op_id>> op;
    switch (sample.op) {
    case op_id::test_no_flow:
        op = std::make_unique<op_no_flow>();
        break;
    case op_id::test_rd:
        op = std::make_unique<op_rd>();
        break;
    case op_id::test_wr:
        op = std::make_unique<op_wr>();
        break;
    case op_id::test_rd_wr:
        op = std::make_unique<op_rd_wr>();
        break;
    default:
        BOOST_REQUIRE(false);
    }
    verdict v;
    for (auto k:
         {soficpp::controller_test::access, soficpp::controller_test::min_subj, soficpp::controller_test::min_obj})
    {
        BOOST_CHECK(acl.test(sample.i_subj, *op, v, k) == sample.test);
    }
}
//! \endcond

/*! \file
 * \test \c ops_acl_null_default -- Test of class soficpp::ops_acl with \c
 * nullptr default inner ACL */
//! \cond
BOOST_AUTO_TEST_CASE(ops_acl_null_default)
{
    ops_acl_t acl{};
    verdict v;
    for (auto k:
         {soficpp::controller_test::access, soficpp::controller_test::min_subj, soficpp::controller_test::min_obj})
    {
        BOOST_CHECK(!acl.test(integrity{}, op_no_flow{}, v, k));
        BOOST_CHECK(!acl.test(integrity{}, op_rd{}, v, k));
        BOOST_CHECK(!acl.test(integrity{}, op_wr{}, v, k));
        BOOST_CHECK(!acl.test(integrity{}, op_rd_wr{}, v, k));
    }
    acl[op_id::test_rd] = std::make_shared<ops_acl_t::acl_t>(ops_acl_t::acl_t{integrity{}});
    for (auto k:
         {soficpp::controller_test::access, soficpp::controller_test::min_subj, soficpp::controller_test::min_obj})
    {
        BOOST_CHECK(!acl.test(integrity{}, op_no_flow{}, v, k));
        BOOST_CHECK(acl.test(integrity{}, op_rd{}, v, k));
        BOOST_CHECK(!acl.test(integrity{}, op_wr{}, v, k));
        BOOST_CHECK(!acl.test(integrity{}, op_rd_wr{}, v, k));
    }
}
//! \endcond

/*! \file
 * \test \c ops_acl_null_acl -- Test of class soficpp::ops_acl with \c nullptr
 * inner ACL in the map */
//! \cond
BOOST_AUTO_TEST_CASE(ops_acl_null_acl)
{
    ops_acl_t acl{ops_acl_t::acl_t{integrity{}}};
    verdict v;
    for (auto k:
         {soficpp::controller_test::access, soficpp::controller_test::min_subj, soficpp::controller_test::min_obj})
    {
        BOOST_CHECK(acl.test(integrity{}, op_no_flow{}, v, k));
        BOOST_CHECK(acl.test(integrity{}, op_rd{}, v, k));
        BOOST_CHECK(acl.test(integrity{}, op_wr{}, v, k));
        BOOST_CHECK(acl.test(integrity{}, op_rd_wr{}, v, k));
    }
    acl[op_id::test_no_flow] = nullptr;
    acl[op_id::test_wr] = nullptr;
    for (auto k:
         {soficpp::controller_test::access, soficpp::controller_test::min_subj, soficpp::controller_test::min_obj})
    {
        BOOST_CHECK(!acl.test(integrity{}, op_no_flow{}, v, k));
        BOOST_CHECK(acl.test(integrity{}, op_rd{}, v, k));
        BOOST_CHECK(!acl.test(integrity{}, op_wr{}, v, k));
        BOOST_CHECK(acl.test(integrity{}, op_rd_wr{}, v, k));
    }
}
//! \endcond
