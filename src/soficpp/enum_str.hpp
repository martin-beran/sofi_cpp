#pragma once

/*! \file
 * \brief Conversion between enums and strings or string views
 *
 * \test in file test_enum_str.cpp
 */

#include <initializer_list>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace soficpp {

namespace impl {

//! Requirements for a type that can be used with soficpp::enum2str() and soficpp::str2enum()
/*! \tparam E type \c bool or an enumeration type */
template <class E> concept can_enum_str = std::is_same_v<E, bool> || std::is_enum_v<E>;

//! A type used to define conversion table between an enumeration and strings
/*! \tparam E an enumeration type, or \c bool
 * \tparam N the number of values in \a E */
template <can_enum_str E, size_t N> struct enum_str_init_t: public std::array<std::pair<E, const char*>, N> {
};

//! A deduction guide for enum_str_init_t
/*! \tparam E an enumeration type, or \c bool
 * \tparam T deduced from the initializer, it must be <tt>std::pair<E, const char*></tt> */
template <can_enum_str E, class... T>
enum_str_init_t(std::pair<E, const char*>, T...) -> enum_str_init_t<E, 1 + sizeof...(T)>;

//! The conversion table between an enumeration and strings
/*! An explicit specialization must be defined for each enumeration type that
 * should be convertible to strings and back by soficpp::enum2str() and
 * soficpp::str2enum()
 * \tparam E  an enumeration type, or \c bool */
template <can_enum_str E> inline constexpr enum_str_init_t<E, 0> enum_str_init{};

//! A helper class that implements conversion between an enumeration and strings
/*! A single instance is created automatically as needed by get().
 * \tparam E  an enumeration type, or \c bool */
template <can_enum_str E> class enum_str {
public:
    //! Gets the single global instance of this class.
    /*! \return the instance */
    static const enum_str& get() {
        static enum_str data;
        return data;
    }
    //! Converts an enumeration value to a string.
    /*! \param[in] e an enumeration value
     * \return the string corresponding to \a e if \a e is defined in the
     * conversion table, the numeric value of \a e converted to a string
     * otherwise */
    std::string convert(E e) const requires std::is_enum_v<E> {
        if (auto p = e2s.find(e); p != e2s.end())
            return std::string{p->second};
        else
            return std::to_string(std::underlying_type_t<E>(e));
    }
    //! Converts an \c bool value to a string.
    /*! \param[in] e a \c bool value
     * \return the string \c "false" or \c "true" corresponding to \a e */
    std::string convert(E e) const requires std::is_same_v<E, bool> {
        if (auto p = e2s.find(e); p != e2s.end())
            return std::string{p->second};
        else
            return std::to_string(e);
    }
    //! Converts a string to an enumeration value.
    /*! \param[in] s a string
     * \return the enumeration value corresponding to \a s
     * \throw std::invalid_argument if no record for \a s is present in the
     * conversion table */
    E convert(std::string_view s) const {
        if (auto p = s2e.find(s); p != s2e.end())
            return p->second;
        else
            throw std::invalid_argument("Unknown enum value");
    }
private:
    //! Constructs the conversion table from enum_str_init.
    explicit enum_str() {
        for (auto&& i: enum_str_init<E>) {
            e2s.emplace(i.first, i.second);
            s2e.emplace(i.second, i.first);
        }
    }
    //! The map for converting enumeration value to strings
    std::map<E, std::string_view> e2s;
    //! The map for converting strings to enumeration values
    std::map<std::string_view, E> s2e;
};

//! Specialization of enum_string_init for type \c bool
template <> inline constexpr enum_str_init_t enum_str_init<bool>{
    std::pair{false, "false"},
    std::pair{true, "true"},
};

} // namespace impl

//! Defines the conversion table between enumeration \a type and strings.
/*! \param[in] type an enumeration type name */
#define SOFICPP_IMPL_ENUM_STR_INIT(type) \
    template <> inline constexpr soficpp::impl::enum_str_init_t soficpp::impl::enum_str_init<type>

//! Defines a single conversion record for a \a value from enumeration \a type.
/*! \param[in] type an enumeration type name
 * \param[in] value a value from \a type */
#define SOFICPP_IMPL_ENUM_STR_VAL(type, value) \
    std::pair{type::value, #value}

//! Converts an enumeration or \c bool value to a string.
/*! \tparam E the type to be converted
 * \param[in] e the value to be converted
 * \return the converted value if a string is defined for \a e in the
 * conversion table; the numeric value of \a e converted to a string otherwise
 *
 * This function is predefined for type \c bool. Before it can be used for an
 * enumeration type \a E, the specialization \c enum_str_init<E> must be
 * defined. It can be done by using macros:
 * \code
 * SOFICPP_IMPL_ENUM_STR_INIT(an_enumeration_type) {
       SOFICPP_IMPL_ENUM_STR_VAL(an_enumeration_type, enum_value_1),
       SOFICPP_IMPL_ENUM_STR_VAL(an_enumeration_type, enum_value_2),
       ...
 * };
 * \endcode
 * \test in file test_enum_str.cpp */
template <impl::can_enum_str E> std::string enum2str(E e)
{
    return impl::enum_str<E>::get().convert(e);
}

//! Converts a string to an enumeration or \c bool value.
/*! \tparam E the target type
 * \param[in] s the value to be converted
 * \return the converted value if a value is defined for \a s in the conversion
 * table
 * \throw std::invalid_argument if \a s is not present in the conversion table
 *
 * This function is predefined for type \c bool. Before it can be used for an
 * enumeration type \a E, the specialization \c enum_str_init<E> must be
 * defined as described in the documentation of enum2str().
 * \test in file test_enum_str.cpp */
template <impl::can_enum_str E> E str2enum(std::string_view s)
{
    return impl::enum_str<E>::get().convert(s);
}

} // namespace soficpp
