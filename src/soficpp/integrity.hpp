#pragma once

/*! \file
 * \brief Storage and manipulation with integrities
 *
 * \test in file test_integrity.cpp
 */

#include <compare>
#include <concepts>

namespace soficpp {

namespace impl {

//! Requirements for a class representing a value from a lattice
/*! Required operations are:
 * \arg \c + = join
 * \arg \c * = meet
 * \tparam T a value from a lattice */
template <class T> concept lattice =
    std::equality_comparable<T> && std::three_way_comparable<T> &&
    requires (T v1, T v2) {
        { v1 + v2 } -> std::same_as<T>; // operation join
        { v1 * v2 } -> std::same_as<T>; // operation meet
    };

//! Requirements for a class representing a value from a bounded lattice
/*! \tparam T a value from a bounded lattice */
template <class T> concept bounded_lattice =
    lattice<T> &&
    requires {
        { T::min() } -> std::same_as<T>; // minimum
        { T::max() } -> std::same_as<T>; // maximum
    };

} // namespace impl

//! Requirements for a class representing an integrity value.
/*! \tparam I an integrity class
 * \test in file test_integrity.cpp */
template <class T> concept integrity =
    impl::bounded_lattice<T> &&
    std::default_initializable<T> && std::destructible<T> &&
    std::copy_constructible<T> && std::assignable_from<T&, T>;

} // namespace soficpp
