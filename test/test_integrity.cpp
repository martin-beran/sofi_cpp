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
    // default constructor
    soficpp::integrity_single s{};
    // conversion to string and output to stream
    BOOST_TEST(s.to_string() == "{}");
    std::ostringstream os;
    os << s;
    BOOST_TEST(os.str() == "{}");
    // copy and assignment
    const soficpp::integrity_single s2 = s;
    BOOST_TEST(s2 == s);
    s = s2;
    BOOST_TEST(s2 == s);
    // comparison
    BOOST_TEST(s == s);
    BOOST_TEST(!(s != s));
    BOOST_TEST(s <= s);
    BOOST_TEST(s >= s);
    BOOST_TEST(!(s < s));
    BOOST_TEST(!(s > s));
    // lattice operations
    BOOST_TEST(soficpp::integrity_single::min() == s);
    BOOST_TEST(soficpp::integrity_single::max() == s);
    BOOST_TEST(s + s == s);
    BOOST_TEST(s * s == s);
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
    // conversion to string and output to stream
    BOOST_TEST(i0.to_string() == "low");
    BOOST_TEST(i1.to_string() == "medium");
    BOOST_TEST(i2.to_string() == "high");
    std::ostringstream os;
    os << i0 << '\n' << i1 << '\n' << i2;
    BOOST_TEST(os.str() == "low\nmedium\nhigh");
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
 * \test \c integrity_linear_int -- Test of class soficpp::integrity_linear
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
    // conversion to string and output to stream
    BOOST_TEST(i0.to_string() == "-1");
    BOOST_TEST(i1.to_string() == "0");
    BOOST_TEST(i2.to_string() == "1");
    BOOST_TEST(i3.to_string() == "2");
    std::ostringstream os;
    os << i0 << '\n' << i1 << '\n' << i2 << '\n' << i3;
    BOOST_TEST(os.str() == "-1\n0\n1\n2");
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

/*! \file
 * \test \c integrity_bitset -- Test of class soficpp::integrity_bitset */
//! \cond
BOOST_AUTO_TEST_CASE(integrity_bitset)
{
    static constexpr size_t n = 5;
    auto to_bitset = [](auto&& s) {
        std::string str{s};
        std::ranges::reverse(str);
        return std::bitset<n>(str);
    };
    using integrity_t = soficpp::integrity_bitset<n>;
    // default constructor
    integrity_t i{};
    BOOST_TEST(i.value() == std::bitset<n>{});
    // constructed from a value
    integrity_t i0{to_bitset("00000")};
    BOOST_TEST(i0.value() == std::bitset<n>{0b00000});
    integrity_t i1{to_bitset("10000")};
    BOOST_TEST(i1.value() == std::bitset<n>{0b00001});
    integrity_t i2{to_bitset("11000")};
    BOOST_TEST(i2.value() == std::bitset<n>{0b00011});
    integrity_t i3{to_bitset("10101")};
    BOOST_TEST(i3.value() == std::bitset<n>{0b10101});
    integrity_t i4{to_bitset("11010")};
    BOOST_TEST(i4.value() == std::bitset<n>{0b01011});
    integrity_t i5{to_bitset("00101")};
    BOOST_TEST(i5.value() == std::bitset<n>{0b10100});
    integrity_t i6{to_bitset("11111")};
    BOOST_TEST(i6.value() == std::bitset<n>{0b11111});
    // conversion to string and output to stream
    BOOST_TEST(i0.to_string() == "00000");
    BOOST_TEST(i1.to_string() == "10000");
    BOOST_TEST(i2.to_string() == "11000");
    BOOST_TEST(i3.to_string() == "10101");
    BOOST_TEST(i4.to_string() == "11010");
    BOOST_TEST(i5.to_string() == "00101");
    BOOST_TEST(i6.to_string() == "11111");
    std::ostringstream os;
    os << i0 << '\n' << i1 << '\n' << i2 << '\n' << i3 << '\n' << i4 << '\n' << i5 << '\n' << i6;
    BOOST_TEST(os.str() == "00000\n10000\n11000\n10101\n11010\n00101\n11111");
    // copy and assignment
    integrity_t c{i2};
    BOOST_TEST(c == i2);
    BOOST_TEST(c != i4);
    c = i4;
    BOOST_TEST(c != i2);
    BOOST_TEST(c == i4);
    // comparison
    BOOST_TEST(i0 == i0);
    BOOST_TEST(i0 != i1);
    BOOST_TEST(i0 != i3);
    BOOST_TEST(i0 != i6);
    BOOST_TEST(i2 == i2);
    BOOST_TEST(i2 != i1);
    BOOST_TEST(i2 != i4);
    BOOST_TEST(i2 != i6);
    BOOST_TEST(i6 != i0);
    BOOST_TEST(i6 != i5);
    BOOST_TEST(i6 == i6);
    BOOST_TEST(i1 <= i1);
    BOOST_TEST(i1 >= i1);
    BOOST_TEST(!(i1 < i1));
    BOOST_TEST(!(i1 > i1));
    BOOST_TEST(i1 <= i4);
    BOOST_TEST(i1 < i4);
    BOOST_TEST(!(i1 >= i4));
    BOOST_TEST(!(i1 > i4));
    BOOST_TEST(i3 >= i5);
    BOOST_TEST(i3 > i5);
    BOOST_TEST(!(i3 <= i5));
    BOOST_TEST(!(i3 < i5));
    BOOST_TEST(!(i3 == i4));
    BOOST_TEST(i3 != i4);
    BOOST_TEST(!(i3 <= i4));
    BOOST_TEST(!(i3 < i4));
    BOOST_TEST(!(i3 >= i4));
    BOOST_TEST(!(i3 > i4));
    // lattice operations
    BOOST_TEST(integrity_t::min() == i0);
    BOOST_TEST(integrity_t::max() == i6);
    BOOST_TEST(i0 + i0 == i0);
    BOOST_TEST(i0 + i1 == i1);
    BOOST_TEST(i0 + i2 == i2);
    BOOST_TEST(i0 + i6 == i6);
    BOOST_TEST(i1 + i3 == i3);
    BOOST_TEST(i1 + i5 == i3);
    BOOST_TEST(i3 + i4 == i6);
    BOOST_TEST(i6 + i4 == i6);
    BOOST_TEST(i0 * i0 == i0);
    BOOST_TEST(i0 * i1 == i0);
    BOOST_TEST(i0 * i2 == i0);
    BOOST_TEST(i0 * i6 == i0);
    BOOST_TEST(i1 * i3 == i1);
    BOOST_TEST(i4 * i5 == i0);
    BOOST_TEST(i3 * i4 == i1);
    BOOST_TEST(i6 * i4 == i4);
}
//! \endcond
