#pragma once

#include "integrity.hpp"

#include <concepts>
#include <functional>
#include <type_traits>

/*! \file
 * \brief Entities (subjects and objects)
 */

namespace soficpp {

//! Requirements for a class representing a SOFI operation
/*! An operation must provide member functions returning \c bool indicating
 * whether the operation is read, write, read-write, or no-flow.
 * \tparam O an operation type */
template <class O> concept operation =
    requires (const O op) {
        { op.is_read() } -> std::same_as<bool>;
        { op.is_write() } -> std::same_as<bool>;
    };

//! A base polymorphic class for operations
/*! It is a no-flow operation */
class operation_base {
public:
    //! Default constructor
    operation_base() = default;
    //! No copy (is polymorphic)
    operation_base(const operation_base&) = delete;
    //! No move (is polymorphic)
    operation_base(operation_base&&) = delete;
    //! No copy (is polymorphic)
    operation_base& operator=(const operation_base&) = delete;
    //! No move (is polymorphic)
    operation_base& operator=(operation_base&&) = delete;
    //! Virtual destructor
    virtual ~operation_base() = default;
    //! Tests if it is a read (or read-write) operation.
    /*! \return \c false, that is, either write or no-flow operation */
    [[nodiscard]] virtual bool is_read() const {
        return false;
    }
    //! Tests if it is a write (or read-write) operation.
    /*! \return \c false, that is, either read or no-flow operation */
    [[nodiscard]] virtual bool is_write() const {
        return false;
    }
    //! Gets the operation name.
    /*! \return the operation name; empty string by default */
    [[nodiscard]] virtual std::string_view name() const {
        return {};
    }
};

static_assert(operation<operation_base>);

//! Requirements for a SOFI verdict, that is, a decision whether an operation may be executed.
/*! A verdict must provide member
 * function allowed() returning \c bool indicating whether the related
 * operation may be performed. It must be contextually convertible to \c bool,
 * yielding the same value as allowed().
 * \tparam V a verdict type */
template <class V> concept verdict =
    requires (const V v) {
        { v.allowed() } -> std::same_as<bool>;
        bool(v);
    };

//! A simple verdict class that stores only an allowed/denied indicator
class simple_verdict {
public:
    //! Stores a verdict.
    /*! \param[in] allowed the verdict to store (denied by default) */
    explicit constexpr simple_verdict(bool allowed = false): _allowed(allowed) {}
    //! Gets the stored verdict.
    /*! \return the verdict */
    explicit constexpr operator bool() const {
        return _allowed;
    }
    //! Gets the stored verdict.
    /*! \return the verdict */
    [[nodiscard]] constexpr bool allowed() const {
        return _allowed;
    }
private:
    //! The stored verdict
    bool _allowed;
};

static_assert(verdict<simple_verdict>);

//! Requirements for an integrity modification function
/*! An itegrity modification function. It is used to modify integrity value \a
 * i passed from entity \a src to entity \a dst during operation \a o with
 * verdict \a v. It returns the modified integrity. The function can assume
 * that <tt>v.allowed()==true</tt>. Integrity \a limit is a hint that the
 * function should make its result less or equal to \ a limit. It depend on the
 * origin of the function if the SOFI engine can trust the function to obey the
 * \a limit.
 * \tparam F a function type
 * \tparam I an integrity type
 * \tparam O a SOFI operation type
 * \tparam V a SOFI verdict type
 * \tparam E an entity type */
template <class F, class I, class O, class V, class E> concept integrity_fun =
    operation<O> && verdict<V> && integrity<I> &&
    requires (F f, const I& i, const I& limit, const O& o, const V& v, const E& src, const E& dst) {
        { f(i, limit, o, v, src, dst) } -> std::same_as<I>;
    };

//! Requirements for a class representing an entity
/*! \tparam T an entity type */
template <class T> concept entity =
    std::is_object_v<T>;

//! A polymorphic function wrapper that satisfies concept integrity_fun
/*! The only difference from its base class std::function is that if it
 * contains no target, the call operator works as the identity function
 * (returns the input integrity) instead of throwing \c std::bad_function_call
 * \tparam I an integrity type
 * \tparam O a SOFI operation type
 * \tparam V a SOFI verdict type
 * \tparam E an entity type */
template <integrity I, operation O, verdict V, entity E>
class unsafe_integrity_fun: std::function<I(const I&, const I&, const O&, const V&, const E&, const E&)> {
    //! The alias for the base class
    using base_t = std::function<I(const I&, const I&, const O&, const V&, const E&, const E&)>;
    using base_t::base_t;
public:
    //! Calls the stored target function
    /*! It returns \a i if there is no target function.
     * \param[in] i an itegrity value to be modified while passed from \a src
     * to \a dst
     * \param[in] limit a hint for result, the should (but is not required to)
     * make its result less or equal to \a limit
     * \param[in] op the operation related to the call of this function
     * \param[in] verdict the verdict for operation \a op; the function can
     * assume <tt>v.allowed()==true</tt>
     * \param[in] src the source entity of \a i
     * \param[in] dst the destination entity of \a i
     * \return the modified integrity */
    I operator()(const I& i, const I& limit, const O& op, const V& verdict, const E& src, const E& dst) const {
        if (*this)
            return base_t::operator()(i, limit, op, verdict, src, dst);
        else
            return i;
    }
};

//! A polymorphic function wrapper that satisfies concept integrity_fun
/*! The only difference from its base class unsafe_integrity_value is that
 * operator()() ensures that the returned value is less or equal to \a limit by
 * returning the value of lattice meet applied to \a limit and the return value
 * of base class operator()().
 *
 * The template parameters are the same as in unsafe_integrity_fun:
 * \tparam I
 * \tparam O
 * \tparam V
 * \tparam E */
template <integrity I, operation O, verdict V, entity E>
class safe_integrity_fun: unsafe_integrity_fun<I, O, V, E> {
    //! The alias for the base class
    using base_t = unsafe_integrity_fun<I, O, V, E>;
    using base_t::base_t;
public:
    //! Calls the stored target function
    /*! The lattice meet operation is applied to \a limit and the result of the
     * base class function. It returns <tt>i*limit</tt> if there is no target
     * function. The parameters are as in unsafe_integrity_fun::operator()():
     * \param[in] i
     * \param[in] limit
     * \param[in] op
     * \param[in] verdict
     * \param[in] src
     * \param[in] dst
     * \return the modified integrity */
    I operator()(const I& i, const I& limit, const O& op, const V& verdict, const E& src, const E& dst) const {
        return base_t::operator()(i, limit, op, verdict, src, dst) * limit;
    }
};

//! A base class template that satisfies concept entity.
/*! \tparam I an integrity type
 * \tparam O an operation type
 * \tparam V a verdict type */
struct basic_entity {
};

static_assert(integrity_fun<unsafe_integrity_fun<integrity_single, operation_base, simple_verdict, basic_entity>,
              integrity_single, operation_base, simple_verdict, basic_entity>);

} // namespace soficpp
