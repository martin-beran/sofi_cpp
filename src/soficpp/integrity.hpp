#pragma once

/*! \file
 * \brief Storage and manipulation with integrities
 *
 * \test in file test_integrity.cpp
 */

#include "soficpp/enum_str.hpp"

#include <compare>
#include <concepts>
#include <ostream>
#include <type_traits>

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
/*! \tparam T an integrity class
 * \test in file test_integrity.cpp */
template <class T> concept integrity =
    impl::bounded_lattice<T> &&
    std::default_initializable<T> && std::destructible<T> &&
    std::copy_constructible<T> && std::assignable_from<T&, T>;

//! The simplest integrity type containing just a single integrity value
/*! \test in file test_integrity.cpp */
class integrity_single {
public:
    //! All integrity_single objects compare as equal.
    /*! Defaulted three-way comparison automatically generates all other
     * comparison operators.
     * \param[in] i compared integrity value
     * \return std::strong_ordering */
    constexpr auto operator<=>(const integrity_single& i) const noexcept = default;
    //! The lattice join operation
    /*! \param[in] i an integrity value
     * \return the result of join */
    constexpr integrity_single operator+([[maybe_unused]] const integrity_single& i) const noexcept {
        return {};
    }
    //! The lattice meet operation
    /*! \param[in] i an integrity value
     * \return the result of meet */
    constexpr integrity_single operator*([[maybe_unused]] const integrity_single& i) const noexcept {
        return {};
    }
    //! The lattice minimum
    /*! \return the least element */
    static constexpr integrity_single min() noexcept {
        return {};
    }
    //! The lattice maximum
    /*! \return the greatest element */
    static constexpr integrity_single max() noexcept {
        return {};
    }
    //! Output of an integrity_single value
    /*! \param[in] os an output stream
     * \param[in] i an integrity value
     * \return \a os */
    friend std::ostream& operator<<(std::ostream& os, [[maybe_unused]] const integrity_single& i)
    {
        os << "{}";
        return os;
    }
};

static_assert(integrity<integrity_single>);
static_assert(std::is_same_v<decltype(integrity_single{} <=> integrity_single{}), std::strong_ordering>);

//! A type that may be used as the parameter of template integrity_linear
/*! \tparam T an integral or enumeration type */
template <class T> concept linear_integrity_value =
    (std::is_integral_v<T> || std::is_enum_v<T>) &&
    std::is_trivially_default_constructible_v<T> && std::is_trivially_destructible_v<T> &&
    std::is_trivially_copy_constructible_v<T> && std::is_trivially_copyable_v<T>;

//! An integrity type with linearly ordered integrity values
/*! \tparam T the underlying type of integrity values
 * \tparam Min the minimum integrity
 * \tparam Max the maximum integrity
 * \test in file test_integrity.cpp */
template <linear_integrity_value T, T Min, T Max> requires (Min <= Max)
class integrity_linear {
public:
    //! Default constructor, sets the value to \a Min
    constexpr integrity_linear() noexcept: val(Min) {}
    //! Creates an integrity from a value of the underlying type.
    /*! \param[in] v the underlying integrity value
     * \throw std::invalid_argument if \a v does not belong to the interval
     * \<\a Min, \a Max\> */
    explicit constexpr integrity_linear(T v): val(v) {
        if (v < Min || v > Max)
            throw std::invalid_argument("Value out of range of integrity_linear");
    }
    //! Compares according to the numeric value.
    /*! Defaulted three-way comparison automatically generates all other
     * comparison operators.
     * \param[in] i compared integrity value
     * \return std::strong_ordering */
    constexpr auto operator<=>(const integrity_linear& i) const noexcept = default;
    //! The lattice join operation
    /*! \param[in] i an integrity value
     * \return the maximum of the arguments */
    constexpr integrity_linear operator+(const integrity_linear& i) const noexcept {
        return integrity_linear{std::max(val, i.val)};
    }
    //! The lattice meet operation
    /*! \param[in] i an integrity value
     * \return the minimum of the arguments */
    constexpr integrity_linear operator*(const integrity_linear& i) const noexcept {
        return integrity_linear{std::min(val, i.val)};
    }
    //! The lattice minimum
    /*! \return \c integrity_linear(Min) */
    static constexpr integrity_linear min() noexcept {
        return integrity_linear{Min};
    }
    //! The lattice maximum
    /*! \return \c integrity_linear(Max) */
    static constexpr integrity_linear max() noexcept {
        return integrity_linear{Max};
    }
    //! Gets the underlying integrity value
    /*! \return the value */
    constexpr T value() const noexcept {
        return val;
    }
private:
    //! The value of this integrity
    T val;
};

static_assert(integrity<integrity_linear<bool, false, true>>);
static_assert(std::is_same_v<
    decltype(integrity_linear<bool, false, true>{} <=> integrity_linear<bool, false, true>{}),
    std::strong_ordering>);
static_assert(integrity<integrity_linear<int, 0, 10>>);
static_assert(std::is_same_v<
    decltype(integrity_linear<int, 0, 10>{} <=> integrity_linear<int, 0, 10>{}),
    std::strong_ordering>);

//! Output of an integrity_linear value
/*! \tparam T the underlying type of integrity values
 * \tparam Min the minimum integrity
 * \tparam Max the maximum integrity
 * \param[in] os an output stream
 * \param[in] i an integrity value
 * \return \a os */
template <class T, T Min, T Max>
inline std::ostream& operator<<(std::ostream& os, const integrity_linear<T, Min, Max>& i)
{
    os << i.value();
    return os;
}

//! Output of an integrity_linear value
/*! \tparam T the underlying enumeration type of integrity values
 * \tparam Min the minimum integrity
 * \tparam Max the maximum integrity
 * \param[in] os an output stream
 * \param[in] i an integrity value
 * \return \a os */
template <class T, T Min, T Max> requires std::is_enum_v<T>
inline std::ostream& operator<<(std::ostream& os, const integrity_linear<T, Min, Max>& i)
{
    os << enum2str(i.value());
    return os;
}

} // namespace soficpp
