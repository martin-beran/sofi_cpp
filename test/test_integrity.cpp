/*! \file
 * \brief Tests of soficpp::integrity
 *
 * It tests declarations in file integrity.hpp.
 */

//! \cond
#include "soficpp/integrity.hpp"

#define BOOST_TEST_MODULE integrity
#include <boost/test/included/unit_test.hpp>

namespace {

struct single {
    auto operator<=>(const single&) const = default;
    single operator+(const single&) const {
        return {};
    }
    single operator*(const single&) const {
        return {};
    }
    static single min() {
        return {};
    }
    static single max() {
        return {};
    }
    friend std::ostream& operator<<(std::ostream& os, const single&) {
        os << "single{}";
        return os;
    }
};

} // namespace
//! \endcond

/*! \file
 * \test \c single_element -- A trivial class that satisfies
 * soficpp::integrity. It contains just a single element. */
//! \cond
BOOST_AUTO_TEST_CASE(single_element)
{
    BOOST_TEST(soficpp::integrity<single>);
    single s{};
    BOOST_TEST(s == s);
    BOOST_TEST(!(s != s));
    BOOST_TEST(s <= s);
    BOOST_TEST(s >= s);
    BOOST_TEST(!(s < s));
    BOOST_TEST(!(s > s));
    BOOST_TEST(single::min() == s);
    BOOST_TEST(single::max() == s);
    BOOST_TEST(s + s == s);
    BOOST_TEST(s * s == s);
    const single s2 = s;
    BOOST_TEST(s2 == s);
    s = s2;
    BOOST_TEST(s2 == s);
}
//! \endcond
