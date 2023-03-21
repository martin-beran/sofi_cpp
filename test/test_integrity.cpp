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

/*! \file
 * \test \c integrity_set -- Test of class soficpp::integrity_set */
//! \cond
BOOST_AUTO_TEST_CASE(integrity_set)
{
    using integrity_t = soficpp::integrity_set<std::string>;
    integrity_t::value_type empty{};
    BOOST_TEST(std::holds_alternative<integrity_t::set_t>(empty));
    BOOST_TEST(std::get<integrity_t::set_t>(empty).empty());
    // default constructor
    integrity_t i{};
    BOOST_TEST(i.value() == empty);
    // constructed from a value
    integrity_t i0{integrity_t::set_t{}};
    BOOST_TEST(i0.value() == integrity_t::value_type{});
    integrity_t i1{integrity_t::set_t{"v1"}};
    BOOST_TEST(i1.value() == integrity_t::value_type{integrity_t::set_t{"v1"}});
    integrity_t i2{integrity_t::set_t{"v1", "v2"}};
    BOOST_TEST((i2.value() == integrity_t::value_type{integrity_t::set_t{"v1", "v2"}}));
    integrity_t i3{integrity_t::set_t{"v1", "v3", "v5"}};
    BOOST_TEST((i3.value() == integrity_t::value_type{integrity_t::set_t{"v1", "v3", "v5"}}));
    integrity_t i4{integrity_t::set_t{"v1", "v2", "v4"}};
    BOOST_TEST((i4.value() == integrity_t::value_type{integrity_t::set_t{"v1", "v2", "v4"}}));
    integrity_t i5{integrity_t::set_t{"v3", "v5"}};
    BOOST_TEST((i5.value() == integrity_t::value_type{integrity_t::set_t{"v3", "v5"}}));
    integrity_t i6{integrity_t::set_t{"v1", "v2", "v3", "v4", "v5"}};
    BOOST_TEST((i6.value() == integrity_t::value_type{integrity_t::set_t{"v1", "v2", "v3", "v4", "v5"}}));
    integrity_t i7{integrity_t::universe{}};
    BOOST_TEST(i7.value() == integrity_t::value_type{integrity_t::universe{}});
    // conversion to string and output to stream
    BOOST_TEST(i0.to_string() == "{}");
    BOOST_TEST(i1.to_string() == "{v1}");
    BOOST_TEST(i2.to_string() == "{v1,v2}");
    BOOST_TEST(i3.to_string() == "{v1,v3,v5}");
    BOOST_TEST(i4.to_string() == "{v1,v2,v4}");
    BOOST_TEST(i5.to_string() == "{v3,v5}");
    BOOST_TEST(i6.to_string() == "{v1,v2,v3,v4,v5}");
    BOOST_TEST(i7.to_string() == "universe");
    std::ostringstream os;
    os << i0 << '\n' << i1 << '\n' << i2 << '\n' << i3 << '\n' << i4 << '\n' << i5 << '\n' << i6 << '\n' << i7;
    BOOST_TEST(os.str() == "{}\n{v1}\n{v1,v2}\n{v1,v3,v5}\n{v1,v2,v4}\n{v3,v5}\n{v1,v2,v3,v4,v5}\nuniverse");
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
    BOOST_TEST(i0 != i7);
    BOOST_TEST(i2 == i2);
    BOOST_TEST(i2 != i1);
    BOOST_TEST(i2 != i4);
    BOOST_TEST(i2 != i6);
    BOOST_TEST(i2 != i7);
    BOOST_TEST(i6 != i0);
    BOOST_TEST(i6 != i5);
    BOOST_TEST(i6 == i6);
    BOOST_TEST(i6 != i7);
    BOOST_TEST(i7 != i0);
    BOOST_TEST(i7 != i5);
    BOOST_TEST(i7 != i6);
    BOOST_TEST(i7 == i7);
    BOOST_TEST(i1 <= i1);
    BOOST_TEST(i1 >= i1);
    BOOST_TEST(!(i1 < i1));
    BOOST_TEST(!(i1 > i1));
    BOOST_TEST(i1 <= i4);
    BOOST_TEST(i1 < i4);
    BOOST_TEST(i1 <= i7);
    BOOST_TEST(i1 < i7);
    BOOST_TEST(!(i1 >= i4));
    BOOST_TEST(!(i1 > i4));
    BOOST_TEST(!(i1 >= i7));
    BOOST_TEST(!(i1 > i7));
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
    BOOST_TEST(i6 <= i7);
    BOOST_TEST(i6 < i7);
    BOOST_TEST(!(i6 >= i7));
    BOOST_TEST(!(i6 > i7));
    BOOST_TEST(!(i7 <= i6));
    BOOST_TEST(!(i7 < i6));
    BOOST_TEST(i7 >= i6);
    BOOST_TEST(i7 > i6);
    BOOST_TEST(i7 <= i7);
    BOOST_TEST(!(i7 < i7));
    BOOST_TEST(i7 >= i7);
    BOOST_TEST(!(i7 > i7));
    // lattice operations
    BOOST_TEST(integrity_t::min() == i0);
    BOOST_TEST(integrity_t::max() == i7);
    BOOST_TEST(i0 + i0 == i0);
    BOOST_TEST(i0 + i1 == i1);
    BOOST_TEST(i0 + i2 == i2);
    BOOST_TEST(i0 + i6 == i6);
    BOOST_TEST(i0 + i7 == i7);
    BOOST_TEST(i1 + i3 == i3);
    BOOST_TEST(i1 + i5 == i3);
    BOOST_TEST(i3 + i4 == i6);
    BOOST_TEST(i6 + i4 == i6);
    BOOST_TEST(i7 + i4 == i7);
    BOOST_TEST(i7 + i7 == i7);
    BOOST_TEST(i0 * i0 == i0);
    BOOST_TEST(i0 * i1 == i0);
    BOOST_TEST(i0 * i2 == i0);
    BOOST_TEST(i0 * i6 == i0);
    BOOST_TEST(i0 * i7 == i0);
    BOOST_TEST(i1 * i3 == i1);
    BOOST_TEST(i4 * i5 == i0);
    BOOST_TEST(i3 * i4 == i1);
    BOOST_TEST(i6 * i4 == i4);
    BOOST_TEST(i7 * i4 == i4);
    BOOST_TEST(i7 * i7 == i7);
}
//! \endcond

/*! \file
 * \test \c integrity_shared -- Thest of class soficpp::integrity_shared */
//! \cond
BOOST_AUTO_TEST_CASE(integrity_shared)
{
    using internal_t = soficpp::integrity_set<std::string>;
    using integrity_t = soficpp::integrity_shared<internal_t>;
    // default constructor
    integrity_t i{};
    BOOST_TEST(i.value() == internal_t());
    {
        integrity_t i2{};
        BOOST_TEST(&i.value() == &i2.value());
    }
}
//! \endcond
