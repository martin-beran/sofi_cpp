/*! \file
 * \brief A dummy test program using Boost::Test
 *
 * This program is used to prepare the test infrastructure.
 *
 * \test \c dummy_boost_pass -- A simple test that always passes
 */

//! \cond
#define BOOST_TEST_MODULE dummy_boost
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(dummy_boost_pass)
{
    BOOST_TEST(true);
}
//! \endcond
