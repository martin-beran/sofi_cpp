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
using set_t = integrity::set_t;
using universe = integrity::universe;
using operation = soficpp::operation_base<op_id>;
using verdict = soficpp::simple_verdict;
using acl = soficpp::ops_acl<integrity, operation, verdict>;
using min_integrity = soficpp::acl_single<integrity, operation, verdict>;
using integrity_fun = soficpp::safe_integrity_fun<integrity, operation>;
using entity = soficpp::basic_entity<integrity, min_integrity, operation, verdict, acl, integrity_fun>;

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
        if (os) {
            *os << "after_test_min execute=" << execute <<
                " i_subj=" << bool(i_subj) << " allow_min_subj=" << allow_min_subj <<
                " i_obj=" << bool(i_obj) << " allow_min_obj=" << allow_min_obj << std::endl;
            if (i_subj)
                *os << "i_subj=" << *i_subj << std::endl;
            if (i_obj)
                *os << "i_obj=" << *i_obj << std::endl;
        }
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
    BOOST_TEST_INFO("e.str=" << e.str.str());
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
    BOOST_TEST_INFO("e.str=" << e.str.str());
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
    BOOST_TEST_INFO("e.str=" << e.str.str());
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
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=0\nafter_test_access execute=0 allow=1\n"
                "after_test_min execute=0 i_subj=0 allow_min_subj=1 i_obj=0 allow_min_obj=1\n");
}
//! \endcond

//! \file
/*! \test \c engine_exe_rd -- Read operation executed */
//! \cond
BOOST_AUTO_TEST_CASE(engine_exe_rd)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i3"}});
    subj.min_integrity() = min_integrity{};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i2"}});
    obj.min_integrity() = min_integrity{};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_rd());
    BOOST_CHECK(v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i2"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=1\nafter_test_access execute=1 allow=1\n"
                "after_test_min execute=1 i_subj=1 allow_min_subj=1 i_obj=0 allow_min_obj=1\n"
                "i_subj={i1}\n"
                "execute_op\n");
}
//! \endcond

//! \file
/*! \test \c engine_notexe_rd -- Read test-only operation allowed */
//! \cond
BOOST_AUTO_TEST_CASE(engine_notexe_rd)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i3"}});
    subj.min_integrity() = min_integrity{};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i2"}});
    obj.min_integrity() = min_integrity{};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_rd(), false);
    BOOST_CHECK(v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i3"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i2"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=0\nafter_test_access execute=0 allow=1\n"
                "after_test_min execute=0 i_subj=1 allow_min_subj=1 i_obj=0 allow_min_obj=1\n"
                "i_subj={i1}\n");
}
//! \endcond

//! \file
/*! \test \c engine_exe_rd_deny_min -- Read operation denied by subject minimum
 * integrity */
//! \cond
BOOST_AUTO_TEST_CASE(engine_exe_rd_deny_min)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i3"}});
    subj.min_integrity() = min_integrity{integrity{set_t{"i1", "i3"}}};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i2"}});
    obj.min_integrity() = min_integrity{};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_rd());
    BOOST_CHECK(!v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(!v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i3"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i2"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=1\nafter_test_access execute=1 allow=1\n"
                "after_test_min execute=1 i_subj=1 allow_min_subj=0 i_obj=0 allow_min_obj=1\n"
                "i_subj={i1}\n");
}
//! \endcond

//! \file
/*! \test \c engine_notexe_rd_deny_min -- Read test-only operation denied by subject minimum
 * integrity */
//! \cond
BOOST_AUTO_TEST_CASE(engine_notexe_rd_deny_min)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i3"}});
    subj.min_integrity() = min_integrity{integrity{set_t{"i1", "i3"}}};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i2"}});
    obj.min_integrity() = min_integrity{};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_rd(), false);
    BOOST_CHECK(!v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(!v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i3"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i2"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=0\nafter_test_access execute=0 allow=1\n"
                "after_test_min execute=0 i_subj=1 allow_min_subj=0 i_obj=0 allow_min_obj=1\n"
                "i_subj={i1}\n");
}
//! \endcond

//! \file
/*! \test \c engine_exe_wr -- Write operation executed */
//! \cond
BOOST_AUTO_TEST_CASE(engine_exe_wr)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i3"}});
    subj.min_integrity() = min_integrity{};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i2"}});
    obj.min_integrity() = min_integrity{};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_wr());
    BOOST_CHECK(v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i3"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=1\nafter_test_access execute=1 allow=1\n"
                "after_test_min execute=1 i_subj=0 allow_min_subj=1 i_obj=1 allow_min_obj=1\n"
                "i_obj={i1}\n"
                "execute_op\n");
}
//! \endcond

//! \file
/*! \test \c engine_notexe_wr -- Write test-only operation allowed */
//! \cond
BOOST_AUTO_TEST_CASE(engine_notexe_wr)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i3"}});
    subj.min_integrity() = min_integrity{};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i2"}});
    obj.min_integrity() = min_integrity{};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_wr(), false);
    BOOST_CHECK(v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i3"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i2"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=0\nafter_test_access execute=0 allow=1\n"
                "after_test_min execute=0 i_subj=0 allow_min_subj=1 i_obj=1 allow_min_obj=1\n"
                "i_obj={i1}\n");
}
//! \endcond

//! \file
/*! \test \c engine_exe_wr_deny_min -- Write operation denied by object minimum
 * integrity */
//! \cond
BOOST_AUTO_TEST_CASE(engine_exe_wr_deny_min)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i3"}});
    subj.min_integrity() = min_integrity{};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i2"}});
    obj.min_integrity() = min_integrity{integrity{set_t{"i1", "i2"}}};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_wr());
    BOOST_CHECK(!v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(!v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i3"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i2"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=1\nafter_test_access execute=1 allow=1\n"
                "after_test_min execute=1 i_subj=0 allow_min_subj=1 i_obj=1 allow_min_obj=0\n"
                "i_obj={i1}\n");
}
//! \endcond

//! \file
/*! \test \c engine_notexe_wr_deny_min -- Write test-only operation denied by object minimum
 * integrity */
//! \cond
BOOST_AUTO_TEST_CASE(engine_notexe_wr_deny_min)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i3"}});
    subj.min_integrity() = min_integrity{};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i2"}});
    obj.min_integrity() = min_integrity{integrity{set_t{"i1", "i2"}}};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_wr(), false);
    BOOST_CHECK(!v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(!v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i3"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i2"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=0\nafter_test_access execute=0 allow=1\n"
                "after_test_min execute=0 i_subj=0 allow_min_subj=1 i_obj=1 allow_min_obj=0\n"
                "i_obj={i1}\n");
}
//! \endcond

//! \file
/*! \test \c engine_exe_rdwr -- Read-write operation executed */
//! \cond
BOOST_AUTO_TEST_CASE(engine_exe_rdwr)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i3", "i4"}});
    subj.min_integrity() = min_integrity{integrity{set_t{"i4"}}};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i2", "i4"}});
    obj.min_integrity() = min_integrity{integrity{set_t{"i1"}}};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_rd_wr());
    BOOST_CHECK(v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i4"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i4"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=1\nafter_test_access execute=1 allow=1\n"
                "after_test_min execute=1 i_subj=1 allow_min_subj=1 i_obj=1 allow_min_obj=1\n"
                "i_subj={i1,i4}\n"
                "i_obj={i1,i4}\n"
                "execute_op\n");
}
//! \endcond

//! \file
/*! \test \c engine_notexe_rdwr -- Read-write test-only operation allowed */
//! \cond
BOOST_AUTO_TEST_CASE(engine_notexe_rdwr)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i3", "i4"}});
    subj.min_integrity() = min_integrity{integrity{set_t{"i4"}}};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i2", "i4"}});
    obj.min_integrity() = min_integrity{integrity{set_t{"i1"}}};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_rd_wr(), false);
    BOOST_CHECK(v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i3", "i4"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i2", "i4"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=0\nafter_test_access execute=0 allow=1\n"
                "after_test_min execute=0 i_subj=1 allow_min_subj=1 i_obj=1 allow_min_obj=1\n"
                "i_subj={i1,i4}\n"
                "i_obj={i1,i4}\n");
}
//! \endcond

//! \file
/*! \test \c engine_exe_rdwr_deny_subj_min -- Read-write operation denied by
 * subject minimum integrity */
//! \cond
BOOST_AUTO_TEST_CASE(engine_exe_rdwr_deny_subj_min)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i3", "i4"}});
    subj.min_integrity() = min_integrity{integrity{set_t{"i3", "i4"}}};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i2", "i4"}});
    obj.min_integrity() = min_integrity{integrity{set_t{"i1"}}};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_rd_wr());
    BOOST_CHECK(!v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(!v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i3", "i4"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i2", "i4"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=1\nafter_test_access execute=1 allow=1\n"
                "after_test_min execute=1 i_subj=1 allow_min_subj=0 i_obj=1 allow_min_obj=1\n"
                "i_subj={i1,i4}\n"
                "i_obj={i1,i4}\n");
}
//! \endcond

//! \file
/*! \test \c engine_notexe_rdwr_deny_subj_min -- Read-write test-only operation denied by
 * subject minimum integrity */
//! \cond
BOOST_AUTO_TEST_CASE(engine_notexe_rdwr_deny_subj_min)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i3", "i4"}});
    subj.min_integrity() = min_integrity{integrity{set_t{"i3", "i4"}}};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i2", "i4"}});
    obj.min_integrity() = min_integrity{integrity{set_t{"i1"}}};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_rd_wr(), false);
    BOOST_CHECK(!v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(!v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i3", "i4"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i2", "i4"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=0\nafter_test_access execute=0 allow=1\n"
                "after_test_min execute=0 i_subj=1 allow_min_subj=0 i_obj=1 allow_min_obj=1\n"
                "i_subj={i1,i4}\n"
                "i_obj={i1,i4}\n");
}
//! \endcond

//! \file
/*! \test \c engine_exe_rdwr_deny_obj_min -- Read-write operation denied by
 * object minimum integrity */
//! \cond
BOOST_AUTO_TEST_CASE(engine_exe_rdwr_deny_obj_min)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i3", "i4"}});
    subj.min_integrity() = min_integrity{integrity{set_t{"i4"}}};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i2", "i4"}});
    obj.min_integrity() = min_integrity{integrity{set_t{"i1", "i2"}}};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_rd_wr());
    BOOST_CHECK(!v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(!v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i3", "i4"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i2", "i4"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=1\nafter_test_access execute=1 allow=1\n"
                "after_test_min execute=1 i_subj=1 allow_min_subj=1 i_obj=1 allow_min_obj=0\n"
                "i_subj={i1,i4}\n"
                "i_obj={i1,i4}\n");
}
//! \endcond

//! \file
/*! \test \c engine_notexe_rdwr_deny_obj_min -- Read-write test-only operation denied by
 * object minimum integrity */
//! \cond
BOOST_AUTO_TEST_CASE(engine_notexe_rdwr_deny_obj_min)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i3", "i4"}});
    subj.min_integrity() = min_integrity{integrity{set_t{"i4"}}};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i2", "i4"}});
    obj.min_integrity() = min_integrity{integrity{set_t{"i1", "i2"}}};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_rd_wr(), false);
    BOOST_CHECK(!v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(!v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i3", "i4"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i2", "i4"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=0\nafter_test_access execute=0 allow=1\n"
                "after_test_min execute=0 i_subj=1 allow_min_subj=1 i_obj=1 allow_min_obj=0\n"
                "i_subj={i1,i4}\n"
                "i_obj={i1,i4}\n");
}
//! \endcond

//! \file
/*! \test \c engine_exe_rdwr_deny_min -- Read-write operation denied by
 * both subject and object minimum integrity */
//! \cond
BOOST_AUTO_TEST_CASE(engine_exe_rdwr_deny_min)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i3", "i4"}});
    subj.min_integrity() = min_integrity{integrity{set_t{"i3", "i4"}}};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i2", "i4"}});
    obj.min_integrity() = min_integrity{integrity{set_t{"i1", "i2"}}};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_rd_wr());
    BOOST_CHECK(!v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(!v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i3", "i4"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i2", "i4"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=1\nafter_test_access execute=1 allow=1\n"
                "after_test_min execute=1 i_subj=1 allow_min_subj=0 i_obj=1 allow_min_obj=0\n"
                "i_subj={i1,i4}\n"
                "i_obj={i1,i4}\n");
}
//! \endcond

//! \file
/*! \test \c engine_notexe_rdwr_deny_min -- Read-write test-only operation denied by
 * both subject and object minimum integrity */
//! \cond
BOOST_AUTO_TEST_CASE(engine_notexe_rdwr_deny_min)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i3", "i4"}});
    subj.min_integrity() = min_integrity{integrity{set_t{"i3", "i4"}}};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i2", "i4"}});
    obj.min_integrity() = min_integrity{integrity{set_t{"i1", "i2"}}};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_rd_wr(), false);
    BOOST_CHECK(!v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(!v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i3", "i4"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i2", "i4"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=0\nafter_test_access execute=0 allow=1\n"
                "after_test_min execute=0 i_subj=1 allow_min_subj=0 i_obj=1 allow_min_obj=0\n"
                "i_subj={i1,i4}\n"
                "i_obj={i1,i4}\n");
}
//! \endcond

//! \file
/*! \test \c engine_exe_rd_test_fun -- Using integrity testing function of the subject in a read operation */
//! \cond
BOOST_AUTO_TEST_CASE(engine_exe_rd_test_fun)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i2", "i3"}});
    subj.min_integrity() = min_integrity{};
    subj.test_fun() = integrity_fun{[si = subj.integrity()](const integrity& i, const integrity& l, const operation&) {
        BOOST_CHECK(si == l);
        return i + integrity{set_t{"i2", "i4"}};
    }};
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i4", "i5"}});
    obj.min_integrity() = min_integrity{};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_rd());
    BOOST_CHECK(v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i2"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i4", "i5"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=1\nafter_test_access execute=1 allow=1\n"
                "after_test_min execute=1 i_subj=1 allow_min_subj=1 i_obj=0 allow_min_obj=1\n"
                "i_subj={i1,i2}\n"
                "execute_op\n");
}
//! \endcond

//! \file
/*! \test \c engine_exe_wr_test_fun -- Using integrity testing function of the object in a write operation */
//! \cond
BOOST_AUTO_TEST_CASE(engine_exe_wr_test_fun)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i2", "i3"}});
    subj.min_integrity() = min_integrity{};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i4", "i5"}});
    obj.min_integrity() = min_integrity{};
    obj.test_fun() = integrity_fun{[oi = obj.integrity()](const integrity& i, const integrity& l, const operation&) {
        BOOST_CHECK(oi == l);
        return i + integrity{set_t{"i2", "i4"}};
    }};
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_wr());
    BOOST_CHECK(v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i2", "i3"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i4"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=1\nafter_test_access execute=1 allow=1\n"
                "after_test_min execute=1 i_subj=0 allow_min_subj=1 i_obj=1 allow_min_obj=1\n"
                "i_obj={i1,i4}\n"
                "execute_op\n");
}
//! \endcond

//! \file
/*! \test \c engine_exe_rdwr_test_fun -- Using integrity testing function of
 * both the subject and the object in a read-write operation */
//! \cond
BOOST_AUTO_TEST_CASE(engine_exe_rdwr_test_fun)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i2", "i3"}});
    subj.min_integrity() = min_integrity{};
    subj.test_fun() = integrity_fun{[si = subj.integrity()](const integrity& i, const integrity& l, const operation&) {
        BOOST_CHECK(si == l);
        return i + integrity{set_t{"i3", "i5"}};
    }};
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1", "i4", "i5"}});
    obj.min_integrity() = min_integrity{};
    obj.test_fun() = integrity_fun{[oi = obj.integrity()](const integrity& i, const integrity& l, const operation&) {
        BOOST_CHECK(oi == l);
        return i + integrity{set_t{"i2", "i4"}};
    }};
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_rd_wr());
    BOOST_CHECK(v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i3"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i4"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=1\nafter_test_access execute=1 allow=1\n"
                "after_test_min execute=1 i_subj=1 allow_min_subj=1 i_obj=1 allow_min_obj=1\n"
                "i_subj={i1,i3}\n"
                "i_obj={i1,i4}\n"
                "execute_op\n");
}
//! \endcond

//! \file
/*! \test \c engine_exe_rd_prov_recv_fun -- Using integrity providing and receiving functions in a read operation */
//! \cond
BOOST_AUTO_TEST_CASE(engine_exe_rd_prov_recv_fun)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1"}});
    subj.min_integrity() = min_integrity{};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun::min();
    subj.recv_fun() = integrity_fun{[](const integrity& i, const integrity& l, const operation&) {
        BOOST_CHECK(i == l);
        return i * integrity{set_t{"i2", "i4"}};
    }};
    obj.integrity(integrity{set_t{"i2", "i3", "i4"}});
    obj.min_integrity() = min_integrity{};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun{[oi = obj.integrity()](const integrity& i, const integrity& l, const operation&) {
        BOOST_CHECK(oi == l);
        return i * integrity{set_t{"i2", "i3"}};
    }};
    obj.recv_fun() = integrity_fun::min();
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_rd());
    BOOST_CHECK(v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i2"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i2", "i3", "i4"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=1\nafter_test_access execute=1 allow=1\n"
                "after_test_min execute=1 i_subj=1 allow_min_subj=1 i_obj=0 allow_min_obj=1\n"
                "i_subj={i2}\n"
                "execute_op\n");
}
//! \endcond

//! \file
/*! \test \c engine_exe_wr_prov_recv_fun -- Using integrity providing and receiving functions in a write operation */
//! \cond
BOOST_AUTO_TEST_CASE(engine_exe_wr_prov_recv_fun)
{
    engine e;
    entity subj;
    entity obj;
    subj.integrity(integrity{set_t{"i1", "i2", "i3", "i4"}});
    subj.min_integrity() = min_integrity{};
    subj.test_fun() = integrity_fun::identity();
    subj.prov_fun() = integrity_fun{[si = subj.integrity()](const integrity& i, const integrity& l, const operation&) {
        BOOST_CHECK(si == l);
        return i * integrity{set_t{"i2", "i3"}};
    }};
    subj.recv_fun() = integrity_fun::min();
    obj.integrity(integrity{set_t{"i1"}});
    obj.min_integrity() = min_integrity{};
    obj.test_fun() = integrity_fun::identity();
    obj.prov_fun() = integrity_fun::min();
    obj.recv_fun() = integrity_fun{[](const integrity& i, const integrity& l, const operation&) {
        BOOST_CHECK(i == l);
        return i * integrity{set_t{"i2", "i4"}};
    }};
    obj.access_ctrl().default_op = std::make_shared<acl::acl_t>(acl::acl_t{integrity{}});
    verdict v = e.operation(subj, obj, op_wr());
    BOOST_CHECK(v.allowed());
    BOOST_CHECK(v.access_test());
    BOOST_CHECK(v.min_test());
    BOOST_CHECK(subj.integrity() == (integrity{set_t{"i1", "i2", "i3", "i4"}}));
    BOOST_CHECK(obj.integrity() == (integrity{set_t{"i1", "i2"}}));
    BOOST_TEST_INFO("e.str=" << e.str.str());
    BOOST_CHECK(e.str.str() == "init_verdict execute=1\nafter_test_access execute=1 allow=1\n"
                "after_test_min execute=1 i_subj=0 allow_min_subj=1 i_obj=1 allow_min_obj=1\n"
                "i_obj={i1,i2}\n"
                "execute_op\n");
}
//! \endcond
