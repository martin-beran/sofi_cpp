#pragma once

/*! \file
 * \brief Storage and manipulation with integrities
 *
 * \test in file test_integrity.cpp
 */

#include "soficpp/enum_str.hpp"

#include <algorithm>
#include <bitset>
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
/*! \tparam T an integrity class */
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
    //! Converts the value to a string
    /*! \return a fixed string representing the integrity value */
    std::string to_string() const {
        return "{}";
    }
    //! Output of an integrity_single value
    /*! \param[in] os an output stream
     * \param[in] i an integrity value
     * \return \a os */
    friend std::ostream& operator<<(std::ostream& os, [[maybe_unused]] const integrity_single& i)
    {
        os << i.to_string();
        return os;
    }
};

static_assert(integrity<integrity_single>);
static_assert(std::is_same_v<decltype(integrity_single{} <=> integrity_single{}), std::strong_ordering>);

namespace impl {

//! A type that may be used as the parameter of template integrity_linear
/*! \tparam T an integral or enumeration type */
template <class T> concept integrity_linear_value =
    (std::is_integral_v<T> || std::is_enum_v<T>) &&
    std::is_trivially_default_constructible_v<T> && std::is_trivially_destructible_v<T> &&
    std::is_trivially_copy_constructible_v<T> && std::is_trivially_copyable_v<T>;

} // namespace impl

//! An integrity type with linearly ordered integrity values
/*! \tparam T the underlying type of integrity values
 * \tparam Min the minimum integrity
 * \tparam Max the maximum integrity
 * \test in file test_integrity.cpp */
template <impl::integrity_linear_value T, T Min, T Max> requires (Min <= Max)
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
    //! Converts the value to a string.
    /*! \return the numeric integrity value */
    std::string to_string() const requires (!std::is_enum_v<T>) {
        return std::to_string(val);
    }
    //! Converts the value to a string.
    /*! \return a string representation of the integrity value, created by
     * enum2str() */
    std::string to_string() const requires std::is_enum_v<T> {
        return enum2str(val);
    }
private:
    //! The value of this integrity
    T val;
    //! Output of an integrity_linear value
    /*! \param[in] os an output stream
     * \param[in] i an integrity value
     * \return \a os */
    friend std::ostream& operator<<(std::ostream& os, const integrity_linear& i) {
        os << i.to_string();
        return os;
    }
};

static_assert(integrity<integrity_linear<bool, false, true>>);
static_assert(std::is_same_v<
    decltype(integrity_linear<bool, false, true>{} <=> integrity_linear<bool, false, true>{}),
    std::strong_ordering>);
static_assert(integrity<integrity_linear<int, 0, 10>>);
static_assert(std::is_same_v<
    decltype(integrity_linear<int, 0, 10>{} <=> integrity_linear<int, 0, 10>{}),
    std::strong_ordering>);

//! An integrity type that uses a set of bits as an integrity value
/*! The values are partially ordered using the subset relation. The lattice
 * operation join and meet are set union and intersection, respectively. The
 * least element is the empty set. the greatest element is the set containing
 * all \a N bits.
 * \tparam N the number of bits in the set
 * \note We treat the \c std::bitset as an array of bits, not as a binary
 * number, therefore the conversion to a string orders the bits from 0th to
 * (N-1)th, which is in reverse compared to the order used by std::bitset
 * constructor and \c std::bitset::to_string().
 * \test in file test_integrity.cpp */
template <size_t N> class integrity_bitset {
public:
    //! The type used to store the integrity value
    using value_type = std::bitset<N>;
    //! Default constructor, creates the empty set.
    constexpr integrity_bitset() noexcept = default;
    //! Creates an integrity from a bitset of the corresponding size.
    /*! \param[in] value selects the bits in the set */
    explicit integrity_bitset(const value_type& value): val(value) {}
    //! Compares the sets for equality.
    /*! The inequality operator is automatically generated.
     * \param[in] i compared integrity value
     * \return whether the two integrities are equal sets */
    bool operator==(const integrity_bitset& i) const noexcept = default;
    //! Checks if one integrity is a subset of the other.
    /*! \param[in] i compared integrity value
     * \return the subset relation of the two integrities */
    std::partial_ordering operator<=>(const integrity_bitset& i) const noexcept {
        if (val == i.val)
            return std::partial_ordering::equivalent; // equal sets
        auto intersect = val;
        intersect &= i.val;
        if (intersect == val)
            return std::partial_ordering::less; // this is a subset of i
        else if (intersect == i.val)
            return std::partial_ordering ::greater; // i is a subset of this
        return std::partial_ordering::unordered; // no one is a subset of the other
    }
    //! The lattice join operation
    /*! \param[in] i an integrity value
     * \return the union of the two sets */
    integrity_bitset operator+(const integrity_bitset& i) const {
        integrity_bitset result{val};
        result.val |= i.val;
        return result;
    }
    //! The lattice meet operation
    /*! \param[in] i an integrity value
     * \return the intersection of the two sets */
    integrity_bitset operator*(const integrity_bitset& i) const {
        integrity_bitset result{val};
        result.val &= i.val;
        return result;
    }
    //! The lattice minimum
    /*! \return the empty set */
    static constexpr integrity_bitset min() noexcept {
        return integrity_bitset{};
    }
    //! The lattice maximum
    /*! \return the set containing all \a N bits */
    static integrity_bitset max() noexcept {
        integrity_bitset result{};
        result.val.set();
        return result;
    }
    //! Gets the underlying set of bits
    /*! \return the set */
    constexpr const value_type& value() const noexcept {
        return val;
    }
    //! Converts the value to a string.
    /*! \return a string of \a N characters \c '0' for each bit not present in
     * the set and \c '1' for each bit of the set, in the order from bit 0 to
     * bit \a N - 1 */
    std::string to_string() const {
        std::string result = val.to_string();
        std::ranges::reverse(result);
        return result;
    }
private:
    //! The value of this integrity
    value_type val{};
    //! Output of an integrity_bitset value
    /*! \param[in] os an output stream
     * \param[in] i an integrity value
     * \return \a os */
    friend std::ostream& operator<<(std::ostream& os, const integrity_bitset& i) {
        os << i.to_string();
        return os;
    }
};

namespace impl {

//! A type that may be used as the parameter of template integrity_set
/*! \tparam T a set element type */
template <class T> concept integrity_set_value =
    (std::is_integral_v<T> || std::is_enum_v<T>) &&
    std::default_initializable<T> && std::destructible<T> &&
    std::copy_constructible<T> && std::assignable_from<T&, T>;

} // namespace impl

template <impl::integrity_set_value T> class integrity_set {
};

} // namespace soficpp
