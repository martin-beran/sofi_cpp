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
    low,
    medium,
    high,
};

} // namespace

SOFICPP_IMPL_ENUM_STR_INIT(integrity_value) {
    SOFICPP_IMPL_ENUM_STR_VAL(integrity_value, low),
    SOFICPP_IMPL_ENUM_STR_VAL(integrity_value, medium),
    SOFICPP_IMPL_ENUM_STR_VAL(integrity_value, high),
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
    using integrity_t = soficpp::integrity_linear<integrity_value, integrity_value::low, integrity_value::high>;
    // default constructor
    integrity_t i{};
    BOOST_TEST(i.value() == integrity_value::low);
    // costructed from a value
    integrity_t i0{integrity_value::low};
    integrity_t i1{integrity_value::medium};
    integrity_t i2{integrity_value::high};
    BOOST_CHECK_THROW(integrity_t{static_cast<integrity_value>(static_cast<int>(integrity_value::low) - 1)},
                      std::invalid_argument);
    BOOST_CHECK_THROW(integrity_t{static_cast<integrity_value>(static_cast<int>(integrity_value::high) + 1)},
                      std::invalid_argument);
    BOOST_TEST(i0.value() == integrity_value::low);
    BOOST_TEST(i1.value() == integrity_value::medium);
    BOOST_TEST(i2.value() == integrity_value::high);
    // copy and assignment
    integrity_t c{i1};
    BOOST_TEST(c == i1);
    BOOST_TEST(c != i2);
    c = i2;
    BOOST_TEST(c != i1);
    BOOST_TEST(c == i2);
    // comparison
    BOOST_TEST(i0 == i0);
    BOOST_TEST(i0 != i1);
    BOOST_TEST(i0 != i2);
    BOOST_TEST(i1 != i0);
    BOOST_TEST(i1 == i1);
    BOOST_TEST(i1 != i2);
    BOOST_TEST(i2 != i0);
    BOOST_TEST(i2 != i1);
    BOOST_TEST(i2 == i2);
    BOOST_TEST(i0 <= i0);
    BOOST_TEST(!(i0 < i0));
    BOOST_TEST(i0 >= i0);
    BOOST_TEST(!(i0 > i0));
    BOOST_TEST(i0 <= i1);
    BOOST_TEST(i0 < i1);
    BOOST_TEST(!(i0 >= i1));
    BOOST_TEST(!(i0 > i1));
    BOOST_TEST(i0 <= i2);
    BOOST_TEST(i0 < i2);
    BOOST_TEST(!(i0 >= i2));
    BOOST_TEST(!(i0 > i2));
    BOOST_TEST(i1 <= i2);
    BOOST_TEST(i1 < i2);
    BOOST_TEST(!(i1 >= i2));
    BOOST_TEST(!(i1 > i2));
    // lattice operations
    BOOST_TEST(integrity_t::min() == i0);
    BOOST_TEST(integrity_t::max() == i2);
    BOOST_TEST(i0 + i0 == i0);
    BOOST_TEST(i0 + i1 == i1);
    BOOST_TEST(i0 + i2 == i2);
    BOOST_TEST(i1 + i0 == i1);
    BOOST_TEST(i1 + i1 == i1);
    BOOST_TEST(i1 + i2 == i2);
    BOOST_TEST(i2 + i0 == i2);
    BOOST_TEST(i2 + i1 == i2);
    BOOST_TEST(i2 + i2 == i2);
    BOOST_TEST(i0 * i0 == i0);
    BOOST_TEST(i0 * i1 == i0);
    BOOST_TEST(i0 * i2 == i0);
    BOOST_TEST(i1 * i0 == i0);
    BOOST_TEST(i1 * i1 == i1);
    BOOST_TEST(i1 * i2 == i1);
    BOOST_TEST(i2 * i0 == i0);
    BOOST_TEST(i2 * i1 == i1);
    BOOST_TEST(i2 * i2 == i2);
}
//! \endcond

/*! \file
 * \test \c integrity_linear_int -- Thest of class soficpp::integrity_linear
 * with int underlying type */
//! \cond
BOOST_AUTO_TEST_CASE(integrity_linear_int)
{
    using integrity_t = soficpp::integrity_linear<int, -1, 2>;
    // default constructor
    integrity_t i{};
    BOOST_TEST(i.value() == -1);
    // costructed from a value
    integrity_t i0{-1};
    integrity_t i1{0};
    integrity_t i2{1};
    integrity_t i3{2};
    BOOST_CHECK_THROW(integrity_t{-2}, std::invalid_argument);
    BOOST_CHECK_THROW(integrity_t{3}, std::invalid_argument);
    BOOST_TEST(i0.value() == -1);
    BOOST_TEST(i1.value() == 0);
    BOOST_TEST(i2.value() == 1);
    BOOST_TEST(i3.value() == 2);
    // copy and assignment
    integrity_t c{i1};
    BOOST_TEST(c == i1);
    BOOST_TEST(c != i2);
    c = i2;
    BOOST_TEST(c != i1);
    BOOST_TEST(c == i2);
    // comparison
    BOOST_TEST(i0 == i0);
    BOOST_TEST(i0 != i1);
    BOOST_TEST(i0 != i2);
    BOOST_TEST(i0 != i3);
    BOOST_TEST(i1 != i0);
    BOOST_TEST(i1 == i1);
    BOOST_TEST(i1 != i2);
    BOOST_TEST(i1 != i3);
    BOOST_TEST(i2 != i0);
    BOOST_TEST(i2 != i1);
    BOOST_TEST(i2 == i2);
    BOOST_TEST(i2 != i3);
    BOOST_TEST(i3 != i0);
    BOOST_TEST(i3 != i1);
    BOOST_TEST(i3 != i2);
    BOOST_TEST(i3 == i3);
    BOOST_TEST(i0 <= i0);
    BOOST_TEST(!(i0 < i0));
    BOOST_TEST(i0 >= i0);
    BOOST_TEST(!(i0 > i0));
    BOOST_TEST(i0 <= i1);
    BOOST_TEST(i0 < i1);
    BOOST_TEST(!(i0 >= i1));
    BOOST_TEST(!(i0 > i1));
    BOOST_TEST(i0 <= i2);
    BOOST_TEST(i0 < i2);
    BOOST_TEST(!(i0 >= i2));
    BOOST_TEST(!(i0 > i2));
    BOOST_TEST(i1 <= i2);
    BOOST_TEST(i1 < i2);
    BOOST_TEST(!(i1 >= i2));
    BOOST_TEST(!(i1 > i2));
    // lattice operations
    BOOST_TEST(integrity_t::min() == i0);
    BOOST_TEST(integrity_t::max() == i3);
    BOOST_TEST(i0 + i0 == i0);
    BOOST_TEST(i0 + i1 == i1);
    BOOST_TEST(i0 + i2 == i2);
    BOOST_TEST(i0 + i3 == i3);
    BOOST_TEST(i1 + i0 == i1);
    BOOST_TEST(i1 + i1 == i1);
    BOOST_TEST(i1 + i2 == i2);
    BOOST_TEST(i1 + i3 == i3);
    BOOST_TEST(i2 + i0 == i2);
    BOOST_TEST(i2 + i1 == i2);
    BOOST_TEST(i2 + i2 == i2);
    BOOST_TEST(i2 + i3 == i3);
    BOOST_TEST(i3 + i0 == i3);
    BOOST_TEST(i3 + i1 == i3);
    BOOST_TEST(i3 + i2 == i3);
    BOOST_TEST(i3 + i3 == i3);
    BOOST_TEST(i0 * i0 == i0);
    BOOST_TEST(i0 * i1 == i0);
    BOOST_TEST(i0 * i2 == i0);
    BOOST_TEST(i0 * i3 == i0);
    BOOST_TEST(i1 * i0 == i0);
    BOOST_TEST(i1 * i1 == i1);
    BOOST_TEST(i1 * i2 == i1);
    BOOST_TEST(i1 * i3 == i1);
    BOOST_TEST(i2 * i0 == i0);
    BOOST_TEST(i2 * i1 == i1);
    BOOST_TEST(i2 * i2 == i2);
    BOOST_TEST(i2 * i3 == i2);
    BOOST_TEST(i3 * i0 == i0);
    BOOST_TEST(i3 * i1 == i1);
    BOOST_TEST(i3 * i2 == i2);
    BOOST_TEST(i3 * i3 == i3);
}
//! \endcond
