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
#include <memory>
#include <ostream>
#include <sstream>
#include <set>
#include <type_traits>
#include <variant>

namespace soficpp {

namespace impl {

//! Requirements for a class representing a value from a lattice
/*! Required operations are:
 * \arg \c + = join
 * \arg \c * = meet
 * \tparam T a value from a lattice */
template <class T> concept lattice =
    std::equality_comparable<T> && std::three_way_comparable<T> &&
    requires (const T v1, const T v2) {
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
/*! A default constructed integrity must have the minimum value.
 * \tparam T an integrity class */
template <class T> concept integrity =
    impl::bounded_lattice<T> && std::regular<T> &&
    requires (const T i) {
        typename T::value_type;
        { i.value() } -> std::same_as<const typename T::value_type&>;
    };

//! The simplest integrity type containing just a single integrity value
/*! \test in file test_integrity.cpp */
class integrity_single {
public:
    //! There is no internal value
    struct value_type {
        //! All values are equal.
        /*! Defaulted three-way comparison automatically generates all other
         * comparison operators.
         * \param[in] i compared value
         * \return std::strong_ordering */
        constexpr auto operator<=>(const value_type& i) const noexcept = default;
        //! Converts the value to a string
        /*! \return a fixed string representing the integrity value */
        [[nodiscard]] std::string to_string() const {
            (void) this; // silence clang-tidy warnings about static member functions
            return "{}";
        }
    };
    static_assert(std::is_same_v<decltype(value_type{} <=> value_type{}), std::strong_ordering>);
    static_assert(value_type{} == value_type{});
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
    //! Gets the underlying integrity value
    /*! \return the value */
    [[nodiscard]] constexpr const value_type& value() const noexcept {
        return val;
    }
    //! Converts the value to a string
    /*! \return a fixed string representing the integrity value */
    std::string to_string() {
        return val.to_string();
    }
private:
    //! The value of this integrity
    value_type val;
    // Output of a value_type value
    /*! It outputs a fixed string representing the integrity value.
     * \param[in] os an output stream
     * \param[in] val a value
     * \return \a os */
    friend std::ostream& operator<<(std::ostream& os, const value_type& val) {
        os << val.to_string();
        return os;
    }
    //! Output of an integrity_single value
    /*! \param[in] os an output stream
     * \param[in] i an integrity value
     * \return \a os */
    friend std::ostream& operator<<(std::ostream& os, [[maybe_unused]] const integrity_single& i)
    {
        os << value_type{};
        return os;
    }
};

static_assert(integrity<integrity_single>);
static_assert(std::is_same_v<decltype(integrity_single{} <=> integrity_single{}), std::strong_ordering>);
static_assert(integrity_single{} == integrity_single{});

namespace impl {

//! A type that may be used as the parameter of template integrity_linear
/*! \tparam T an integral or enumeration type */
template <class T> concept integrity_linear_value = std::is_integral_v<T> || std::is_enum_v<T>;

} // namespace impl

//! An integrity type with linearly ordered integrity values
/*! \tparam T the underlying type of integrity values
 * \tparam Min the minimum integrity
 * \tparam Max the maximum integrity
 * \test in file test_integrity.cpp */
template <impl::integrity_linear_value T, T Min, T Max> requires (Min <= Max)
class integrity_linear {
public:
    //! The type used to store the integrity value
    using value_type = T;
    //! Default constructor, sets the value to \a Min
    constexpr integrity_linear() noexcept: val(Min) {}
    //! Creates an integrity from a value of the underlying type.
    /*! \param[in] v the underlying integrity value
     * \throw std::invalid_argument if \a v does not belong to the interval
     * \<\a Min, \a Max\> */
    explicit constexpr integrity_linear(value_type v): val(v) {
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
    [[nodiscard]] constexpr const value_type& value() const noexcept {
        return val;
    }
    //! Converts the value to a string.
    /*! \return the numeric integrity value */
    [[nodiscard]] std::string to_string() const requires (!std::is_enum_v<value_type>) {
        return std::to_string(val);
    }
    //! Converts the value to a string.
    /*! \return a string representation of the integrity value, created by
     * enum2str() */
    [[nodiscard]] std::string to_string() const requires std::is_enum_v<value_type> {
        return enum2str(val);
    }
private:
    //! The value of this integrity
    value_type val;
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
    [[nodiscard]] constexpr const value_type& value() const noexcept {
        return val;
    }
    //! Converts the value to a string.
    /*! \return a string of \a N characters \c '0' for each bit not present in
     * the set and \c '1' for each bit of the set, in the order from bit 0 to
     * bit \a N - 1 */
    [[nodiscard]] std::string to_string() const {
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

static_assert(integrity<integrity_bitset<10>>);

namespace impl {

//! A type that may be used as the parameter of template integrity_set
/*! \tparam T a set element type */
template <class T> concept integrity_set_value = std::regular<T>;

} // namespace impl

//! An integrity type that uses a set of values as an integrity value
/*! The values are partially ordered using the subset relation. The lattice
 * operation join and meet are set union and intersection, respectively. The
 * least element is the empty set. the greatest element is the special value
 * \ref universe, representing a set containing all possible values.
 * \tparam T the type of elements of an integrity value
 * \test in file test_integrity.cpp */
template <impl::integrity_set_value T> class integrity_set {
public:
    //! An empty structure representing the maximum integrity of integrity_set
    /*! It is treated as strictly greater than any subset of values of type \a
     * T. Even if the number of possible values of \a T, an hence the number of
     * its subsets, is finite, \ref universe is still strictly greater than a
     * set explicitly enumerating all values of \a T. */
    struct universe {
        //! Equality of \ref universe values
        /*! \param[in] u another value
         * \return always \c true */
        constexpr bool operator==([[maybe_unused]] const universe& u) const = default;
    };
    static_assert(universe{} == universe{});
    //! The type of integrity_set values that are not equal to \ref universe
    using set_t = std::set<T>;
    //! The type used to store the integrity_set value
    using value_type = std::variant<set_t, universe>;
    //! Default constructor, creates the empty set.
    constexpr integrity_set() = default;
    //! Creates an integrity from a value_type.
    /*! \param[in] value the value of the integrity */
    explicit integrity_set(value_type value): val(std::move(value)) {}
    //! Creates an integrity from a set_t.
    /*! The created set will have a value different from max().
     * \param[in] value the value of the integrity */
    explicit integrity_set(set_t value): val(std::move(value)) {}
    //! Creates an integrity equal to max()
    /*! \param[in] value the \ref universe value */
    explicit integrity_set(universe value): val(value) {}
    //! Compares the sets for equality.
    /*! The inequality operator is automatically generated.
     * \param[in] i compared integrity value
     * \return whether the two integrities are either equal sets, or both \ref
     * universe */
    bool operator==(const integrity_set& i) const {
        if (std::holds_alternative<universe>(val))
            return std::holds_alternative<universe>(i.val);
        else if (std::holds_alternative<universe>(i.val))
            return false;
        else
            return std::get<set_t>(val) == std::get<set_t>(i.val);
    }
    //! Checks if one integrity is a subset of the other.
    /*! Value \ref universe is treated as equal to itself and a proper superset
     * of any set_t.
     * \param[in] i compared integrity value
     * \return the subset relation of the two integrities */
    std::partial_ordering operator<=>(const integrity_set& i) const {
        if (std::holds_alternative<universe>(val)) {
            if (std::holds_alternative<universe>(i.val))
                return std::partial_ordering::equivalent;
            else
                return std::partial_ordering::greater;
        } else if (std::holds_alternative<universe>(i.val))
            return std::partial_ordering::less;
        else {
            const auto& v_set = std::get<set_t>(val);
            const auto& i_set = std::get<set_t>(i.val);
            bool v_subset_of_i = true;
            bool i_subset_of_v = true;
            for (const auto& v: v_set)
                if (!i_set.contains(v)) {
                    v_subset_of_i = false;
                    break;
                }
            for (const auto& i: i_set)
                if (!v_set.contains(i)) {
                    i_subset_of_v = false;
                    break;
                }
            if (v_subset_of_i) {
                if (i_subset_of_v)
                    return std::partial_ordering::equivalent;
                else
                    return std::partial_ordering::less;
            } else {
                if (i_subset_of_v)
                    return std::partial_ordering::greater;
                else
                    return std::partial_ordering::unordered;
            }
        }
    }
    //! The lattice join operation
    /*! The join of \ref universe with anything is \ref universe.
     * \param[in] i an integrity value
     * \return the union of the two sets */
    integrity_set operator+(const integrity_set& i) const {
        if (std::holds_alternative<universe>(val) || std::holds_alternative<universe>(i.val))
            return integrity_set{universe{}};
        integrity_set result{};
        auto& r = std::get<set_t>(result.val);
        for (const auto& v: std::get<set_t>(val))
            r.insert(v);
        for (const auto& i: std::get<set_t>(i.val))
            r.insert(i);
        return result;
    }
    //! The lattice meet operation
    /*! The meet of \ref universe with anything is the other value.
     * \param[in] i an integrity value
     * \return the intesection of the two sets */
    integrity_set operator*(const integrity_set& i) const {
        if (std::holds_alternative<universe>(val))
            return i;
        if (std::holds_alternative<universe>(i.val))
            return *this;
        integrity_set result{};
        auto& r = std::get<set_t>(result.val);
        const auto& v_set = std::get<set_t>(val);
        const auto& i_set = std::get<set_t>(i.val);
        const auto& set1 = v_set.size() < i_set.size() ? v_set : i_set;
        const auto& set2 = v_set.size() < i_set.size() ? i_set : v_set;
        // interate over the smaller set in O(n), test membership in the greater set in O(log(n))
        for (const auto& v: set1)
            if (set2.contains(v))
                r.insert(v);
        return result;
    }
    //! The lattice minimum
    /*! \return the empty set */
    static constexpr integrity_set min() {
        return integrity_set{};
    }
    //! The lattice maximum
    /*! \return \ref universe */
    static constexpr integrity_set max() {
        return integrity_set{universe{}};
    }
    //! Gets the underlying set
    /*! \return the set */
    [[nodiscard]] constexpr const value_type& value() const noexcept {
        return val;
    }
    //! Converts the value to a string.
    /*! It uses stream operator \c << for creating strings from set values.
     * \return a comma-separated list of string representations of individual
     * set values, in the order defined by set_t, enclosed in braces, or the
     * string \c "universe" */
    [[nodiscard]] std::string to_string() const {
        std::ostringstream os;
        os << *this;
        return os.str();
    }
private:
    //! The value of this integrity
    value_type val{};
    //! Output of a value_type value
    /*! \param[in] os an output stream
     * \param[in] val a value
     * \return \a os */
    friend std::ostream& operator<<(std::ostream& os, const typename integrity_set::value_type& val) {
        if (std::holds_alternative<universe>(val)) {
            os << "universe";
        } else {
            os << '{';
            for (bool first = true; auto&& v: std::get<set_t>(val)) {
                if (first)
                    first = false;
                else
                    os << ',';
                os << v;
            }
            os << '}';
        }
        return os;
    }
    //! Output of an integrity_set value
    /*! \tparam T the type of elements of an integrity value
     * \param[in] os an output stream
     * \param[in] i an integrity value
     * \return \a os */
    friend std::ostream& operator<<(std::ostream& os, const integrity_set& i) {
        os << i.val;
        return os;
    }
};

static_assert(integrity<integrity_set<std::string>>);

//! An integrity type that holds another integrity type and shares it when copied
/*! It holds a shared pointer to an integrity object of type \a T internally.
 * Copying an integrity_cow object copies only the pointer, not the target
 * object. Moving from an integrity_cow object sets the internal object to \c
 * nullptr, therefore after a move, it is not safe to access any member of the
 * object (except calling the destructor).
 * \tparam T the internal integrity type
 * \test in file test_integrity.cpp */
template <integrity T> class integrity_shared {
public:
    //! The type of the internal integrity object
    using value_type = T;
    //! The default constructor
    /*! All objects created by this constructor share a single default
     * constructed internal object. */
    integrity_shared(): val(nullptr) {
        static std::shared_ptr<value_type> p = std::make_shared<value_type>();
        val = p;
    }
    //! A constructor creates the internal integrity object.
    /*! Constructor arguments are forwarded to the constructor of the internal
     * object.
     * \param[in] a constructor arguments of integrity type \a T
     * \throw any exception thrown by \c std::make_shared() or by the
     * constructor of \a T */
    template <class... Args> explicit integrity_shared(Args... a):
        val{std::make_shared<value_type>(std::forward<Args>(a)...)} {}
    //! A constructor that creates the internal object by move from an object of type \a T
    /*! \param[in] i an integrity of type \a T */
    explicit integrity_shared(value_type&& i): val{std::make_shared<value_type>(std::move(i))} {}
    //! Compares the internal objects.
    /*! \param[in] i compared integrity value
     * \return whether the two integrities are equal */
    bool operator==(const integrity_shared& i) const {
        return *val == *i.val;
    }
    //! Compares the internal objects.
    /*! \param[in] i compared integrity value
     * \return the result of comparison, of the same type as returned by the
     * three-way comparison operator of the internal objects */
    auto operator<=>(const integrity_shared& i) const {
        return *val <=> *i.val;
    }
    //! The lattice join operation
    /*! It computes the join of the internal objects. If the result is the same
     * as one of its arguments, it shares the internal object with that
     * argument.
     * \param[in] i an integrity value
     * \return the result of join */
    integrity_shared operator+(const integrity_shared& i) const {
        value_type result = *val + *i.val;
        if (result == *val)
            return *this;
        else if (result == *i.val)
            return i;
        else
            return integrity_shared{std::move(result)};
    }
    //! The lattice meet operation
    /*! It computes the meet of the internal objects. If the result is the same
     * as one of its arguments, it shares the internal object with that
     * argument.
     * \param[in] i an integrity value
     * \return the result of join */
    integrity_shared operator*(const integrity_shared& i) const {
        value_type result = *val * *i.val;
        if (result == *val)
            return *this;
        else if (result == *i.val)
            return i;
        else
            return integrity_shared{std::move(result)};
    }
    //! The lattice minimum
    /*! It returns the minimum of integrity type \a T. All objects returned by
     * this function share a single internal object.
     * \return the minimum integrity */
    static integrity_shared min() {
        static std::shared_ptr<value_type> p = std::make_shared<value_type>(value_type::min());
        return integrity_shared(p);
    }
    //! The lattice maximum
    /*! It returns the maximum of integrity type \a T. All objects returned by
     * this function share a single internal object.
     * \return the maximum integrity */
    static integrity_shared max() {
        static std::shared_ptr<value_type> p = std::make_shared<value_type>(value_type::max());
        return integrity_shared(p);
    }
    //! Gets the internal integrity
    /*! \return the internal integrity object */
    [[nodiscard]] const value_type& value() const noexcept {
        return *val;
    }
    //! Converts the value to a string.
    /*! \return the string representation of the internal integrity object */
    [[nodiscard]] std::string to_string() const {
        return val->to_string();
    }
private:
    //! The constructor used internally by min() and max()
    /*! It shares the internal object with \a p.
     * \param[in] p a shared pointer to the internal object */
    explicit integrity_shared(std::shared_ptr<value_type> p): val{p} {}
    //! The value of this integrity, it is never \c nullptr.
    std::shared_ptr<value_type> val;
    //! Output of an integrity_shared value
    /*! It calls \c operator<<() of the internal object.
     * \param[in] os an output stream
     * \param[in] i an integrity value
     * \return \a os */
    friend std::ostream& operator<<(std::ostream& os, const integrity_shared<value_type>& i) {
        os << *i.val;
        return os;
    }
};

static_assert(integrity<integrity_shared<integrity_set<std::string>>>);

} // namespace soficpp
