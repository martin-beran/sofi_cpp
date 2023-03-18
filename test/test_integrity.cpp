/*! \file
 * \brief Tests of soficpp::integrity
 *
 * It tests declarations in file integrity.hpp.
 */

//! \cond
#include "soficpp/soficpp.hpp"
#include <stdexcept>

#define BOOST_TEST_MODULE integrity
#include <boost/test/included/unit_test.hpp>
//! \endcond

/*! \file
 * \test \c integrity_single -- Test of class soficpp::integrity_single */
//! \cond
BOOST_AUTO_TEST_CASE(integrity_single)
{
    soficpp::integrity_single s{};
    BOOST_TEST(s == s);
    BOOST_TEST(!(s != s));
    BOOST_TEST(s <= s);
    BOOST_TEST(s >= s);
    BOOST_TEST(!(s < s));
    BOOST_TEST(!(s > s));
    BOOST_TEST(soficpp::integrity_single::min() == s);
    BOOST_TEST(soficpp::integrity_single::max() == s);
    BOOST_TEST(s + s == s);
    BOOST_TEST(s * s == s);
    const soficpp::integrity_single s2 = s;
    BOOST_TEST(s2 == s);
    s = s2;
    BOOST_TEST(s2 == s);
}

namespace {

enum class integrity_value: int {
    minimum,
    middle,
    maximum,
};

} // namespace

SOFICPP_IMPL_ENUM_STR_INIT(integrity_value) {
    SOFICPP_IMPL_ENUM_STR_VAL(integrity_value, minimum),
    SOFICPP_IMPL_ENUM_STR_VAL(integrity_value, middle),
    SOFICPP_IMPL_ENUM_STR_VAL(integrity_value, maximum),
};

namespace {

std::ostream& operator<<(std::ostream& os, integrity_value v)
{
    os << soficpp::enum2str(v);
    return os;
}

} // namespace
//! \endcond

/*! \file
 * \test \c integrity_linear_enum -- Test of class soficpp::integrity_linear
 * with an enumeration underlying type */
//! \cond
BOOST_AUTO_TEST_CASE(integrity_linear_enum)
{
    using integrity_t = soficpp::integrity_linear<integrity_value, integrity_value::minimum, integrity_value::maximum>;
    integrity_t i{};
    BOOST_TEST(i.value() == integrity_value::minimum);
    integrity_t i0{integrity_value::minimum};
    integrity_t i1{integrity_value::middle};
    integrity_t i2{integrity_value::maximum};
    BOOST_CHECK_THROW(integrity_t{static_cast<integrity_value>(static_cast<int>(integrity_value::minimum) - 1)},
                      std::invalid_argument);
    BOOST_CHECK_THROW(integrity_t{static_cast<integrity_value>(static_cast<int>(integrity_value::maximum) + 1)},
                      std::invalid_argument);
    BOOST_TEST(i0.value() == integrity_value::minimum);
    BOOST_TEST(i1.value() == integrity_value::middle);
    BOOST_TEST(i2.value() == integrity_value::maximum);
}
//! \endcond
