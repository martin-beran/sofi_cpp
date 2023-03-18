/*! \file
 * \brief Tests of soficpp::enum2str() and soficpp::str2enum()
 *
 * It tests functions from file enum_str.hpp.
 */

//! \cond
#include "soficpp/soficpp.hpp"

#define BOOST_TEST_MODULE enum_str
#include <boost/test/included/unit_test.hpp>
//! \endcond

/*! \file
 * \test \c bool2str -- Test converting \c bool values to strings */
//! \cond
BOOST_AUTO_TEST_CASE(bool2str)
{
    BOOST_TEST(soficpp::enum2str(false) == "false");
    BOOST_TEST(soficpp::enum2str(true) == "true");
}
//! \endcond

/*! \file
 * \test \c str2bool -- Test converting strings to \c bool values */
//! \cond
BOOST_AUTO_TEST_CASE(str2bool)
{
    BOOST_TEST(soficpp::str2enum<bool>("false") == false);
    BOOST_TEST(soficpp::str2enum<bool>("true") == true);
    BOOST_CHECK_THROW(soficpp::str2enum<bool>("unknown"), std::invalid_argument);
}

namespace {

enum class test_enum: int {
    val0,
    val1,
    val2,
};

} // namespace

SOFICPP_IMPL_ENUM_STR_INIT(test_enum) {
    SOFICPP_IMPL_ENUM_STR_VAL(test_enum, val0),
    SOFICPP_IMPL_ENUM_STR_VAL(test_enum, val1),
    SOFICPP_IMPL_ENUM_STR_VAL(test_enum, val2),
};

namespace {

std::ostream& operator<<(std::ostream& os, test_enum e)
{
    os << soficpp::enum2str(e);
    return os;
}

} // namespace
//! \endcond

/*! \file
 * \test \c enum2str -- Test converting enumeration values to strings */
//! \cond
BOOST_AUTO_TEST_CASE(enum2str)
{
    BOOST_TEST(soficpp::enum2str(test_enum::val0) == "val0");
    BOOST_TEST(soficpp::enum2str(test_enum::val1) == "val1");
    BOOST_TEST(soficpp::enum2str(test_enum::val2) == "val2");
    BOOST_TEST(soficpp::enum2str(static_cast<test_enum>(-1)) == "-1");
    BOOST_TEST(soficpp::enum2str(static_cast<test_enum>(3)) == "3");
}
//! \endcond

/*! \file
 * \test \c str2enum -- Test converting strings to enumeration values */
//! \cond
BOOST_AUTO_TEST_CASE(str2enum)
{
    BOOST_TEST(soficpp::str2enum<test_enum>("val0") == test_enum::val0);
    BOOST_TEST(soficpp::str2enum<test_enum>("val1") == test_enum::val1);
    BOOST_TEST(soficpp::str2enum<test_enum>("val2") == test_enum::val2);
    BOOST_CHECK_THROW(soficpp::str2enum<test_enum>("false"), std::invalid_argument);
}
//! \endcond
