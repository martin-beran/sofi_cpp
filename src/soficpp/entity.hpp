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
 * \tparam T an operation type */
template <class T> concept operation =
    requires (const T op) {
        { op.is_read() } -> std::same_as<bool>;
        { op.is_write() } -> std::same_as<bool>;
    };

//! A base polymorphic class for operations
/*! It is a no-flow operation, identified by an enumeration and by a string
 * name.
 * \tparam E an enumeration of operation identifiers */
template <class E> requires std::is_enum_v<E>
class operation_base {
public:
    //! The type of operation identifiers
    using id_t = E;
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
    //! Gets the operation identifier.
    /*! \return the operation identifier; default-constructed enum value by
     * default */
    [[nodiscard]] virtual id_t id() const {
        return {};
    }
    //! Gets the operation name.
    /*! \return the operation name; empty string by default */
    [[nodiscard]] virtual std::string_view name() const {
        return {};
    }
};

namespace impl {

//! A dummy enum usable as the template argument of operation_base
/*! It is used only in several following static asserts */
enum class operation_base_dummy_id {
    single, //!< the only enumerator
};

}

static_assert(operation<operation_base<impl::operation_base_dummy_id>>);

//! Requirements for a SOFI verdict, that is, a decision whether an operation may be executed.
/*! A verdict must
 * \arg provide member function allowed() returning \c bool indicating whether
 * the related operation may be performed; it may return \c true only if both
 * \c access_test(true) and \c min_test(true) have been called on this
 * verdict object
 * \arg be contextually convertible to \c bool, yielding the same value as
 * allowed()
 * \arg provide member fuction access_test(), which stores a result of
 * evaluation of an access_controller
 * \arg provide member function min_test(), which stores a result of a minimum
 * integrity test
 * \tparam T a verdict type */
template <class T> concept verdict =
    std::semiregular<T> &&
    requires (const T cv, T v, bool test) {
        { cv.allowed() } -> std::same_as<bool>;
        bool(cv);
        v.access_test(test);
        v.min_test(test);
    };

//! A simple verdict class that stores only the arguments of access_check() and min_check().
class simple_verdict {
public:
    //! Default constructor, initializes the verdict to denied.
    explicit constexpr simple_verdict() = default;
    //! Gets the stored verdict.
    /*! \return the verdict */
    explicit constexpr operator bool() const {
        return allowed();
    }
    //! Gets the stored verdict.
    /*! \return the verdict */
    [[nodiscard]] constexpr bool allowed() const {
        return _access && _min;
    }
    //! Sets the result of access_controller evaluation.
    /*! \param[in] val the value to store */
    void access_test(bool val) noexcept {
        _access = val;
    }
    //! Sets the result of minimum integrity test.
    /*! \param[in] val the value to store */
    void min_test(bool val) noexcept {
        _min = val;
    }
private:
    //! The stored result of evaluation of an access_controller
    bool _access = false;
    //! The stored result of evaluation of a minimum integrity test
    bool _min = false;
};

static_assert(verdict<simple_verdict>);

//! Requirements for a SOFI access access controller
/*! Member function \c test() of the object's access controller is evaluated
 * with the subject integrity in order to decide if an operation is allowed.
 * \tparam T an access controller type
 * \tparam I an integrity type
 * \tparam O an operation type
 * \tparam V a verdict type */
template <class T, class I, class O, class V> concept access_controller =
    integrity<I> && operation<O> && verdict<V> &&
    requires (T ctrl, const I subj, const O op, V v) {
        { ctrl.test(subj, op, v) } -> std::same_as<bool>;
    };

//! Requirements for an integrity modification function
/*! An integrity modification function object. It is used to modify integrity
 * value \a i passed from a source entity to a destination entity during
 * operation \a o. It returns the modified integrity.
 *
 * Integrity \a limit is a hint that the function should make its result less
 * or equal to \a limit. It depends on the origin of the function if the SOFI
 * engine can trust the function to be \e safe, that is to alaways obey the \a
 * limit. Safety of the function is returned by its member function \c safe().
 *
 * In addition, the function object must have static member fuctions:
 * \arg \c min() -- provides a function that always returns the minimum integrity value
 * \arg \c identity() -- provides a function that behaves as the identity
 * function limited by \a limit
 * \arg \c  max() -- provides a function that always returns \a limit
 * \tparam F a function type
 * \tparam I an integrity type
 * \tparam O a SOFI operation type */
template <class F, class I, class O> concept integrity_function =
    integrity<I> && operation<O> && std::movable<F> &&
    requires (const F f, const I i, const I limit, const O o) {
        { f(i, limit, o) } -> std::same_as<I>;
        { f.safe() } -> std::same_as<bool>;
        { F::min() } -> std::same_as<F>;
        { F::identity() } -> std::same_as<F>;
        { F::max() } -> std::same_as<F>;
    };

//! A polymorphic function wrapper that satisfies concept integrity_function
/*! It differs from its base class std::function in that if it
 * contains no target, the call operator works as the identity function
 * (returns the input integrity) instead of throwing \c std::bad_function_call.
 * Its safety status is dynamically changeable, always created as unsafe. If
 * changed to safe, the caller must ensure that the target function is safe.
 * \tparam I an integrity type
 * \tparam O a SOFI operation type */
template <integrity I, operation O>
class dyn_integrity_fun: std::function<I(const I&, const I&, const O&)> {
    //! The alias for the base class
    using base_t = std::function<I(const I&, const I&, const O&)>;
    using base_t::base_t;
public:
    //! Calls the stored target function
    /*! \param[in] i an itegrity value to be modified while passed from a
     * source to a destination object
     * \param[in] limit a hint for result, the should (but is not required to)
     * make its result less or equal to \a limit
     * \param[in] op the operation related to the call of this function
     * \return the result of the target function; \a i if there is no target
     * function and this function is unsafe; <tt>i*limit</tt> if there is no
     * target function and this function is safe */
    I operator()(const I& i, const I& limit, const O& op) const {
        if (*this)
            return base_t::operator()(i, limit, op);
        else
            return safe() ? i * limit : i;
    }
    //! Gets the safety status of this function
    /*! \return the safety status */
    [[nodiscard]] bool safe() const noexcept {
        return _safe;
    }
    //! Sets the safety status of this function
    /*! \param[in] val the new safety status */
    void safe(bool val) noexcept {
        _safe = val;
    }
    //! Gets a function always returning minimum identity
    /*! \return the function object; always safe */
    static dyn_integrity_fun min() {
        dyn_integrity_fun f{[](auto&&, auto&&, auto&&) {
            return I::min();
        }};
        f.safe(true);
        return f;
    }
    //! Gets an identity function
    /*! \return the function object; always safe */
    static dyn_integrity_fun identity() {
        dyn_integrity_fun f{};
        f.safe(true);
        return f;
    }
    //! Gets a function always returning the limit indentity
    /*! \return the function object; always safe */
    static dyn_integrity_fun max() {
        dyn_integrity_fun f{[](auto&&, auto&& limit, auto&&) {
            return limit;
        }};
    }
private:
    //! The stored safety status of this function
    bool _safe = false;
};

static_assert(integrity_function<dyn_integrity_fun<integrity_single, operation_base<impl::operation_base_dummy_id>>,
              integrity_single, operation_base<impl::operation_base_dummy_id>>);

//! A polymorphic function wrapper that satisfies concept integrity_function
/*! It differs from its base class std::function in that if it
 * contains no target, the call operator works as the identity function
 * (returns the input integrity) instead of throwing \c std::bad_function_call.
 * Its safety status is fixed by a template parameter. If marked as safe, the
 * origin of its target function must ensure that the target function is safe.
 * \tparam I an integrity type
 * \tparam O a SOFI operation type
 * \tparam Safe safety of this object */
template <integrity I, operation O, bool Safe = false>
class integrity_fun: std::function<I(const I&, const I&, const O&)> {
    //! The alias for the base class
    using base_t = std::function<I(const I&, const I&, const O&)>;
    using base_t::base_t;
public:
    //! Calls the stored target function
    /*! \param[in] i an itegrity value to be modified while passed from a
     * source to a destination object
     * \param[in] limit a hint for result, the should (but is not required to)
     * make its result less or equal to \a limit
     * \param[in] op the operation related to the call of this function
     * \return the result of the target function; \a i if there is no target
     * function and this function is unsafe; <tt>i*limit</tt> if there is no
     * target function and this function is safe */
    I operator()(const I& i, const I& limit, const O& op) const {
        if (*this)
            return base_t::operator()(i, limit, op);
        else 
            return safe() ? i * limit : i;
    }
    //! Gets the safety status of this function
    /*! \return the safety status */
    [[nodiscard]] constexpr bool safe() const noexcept {
        return Safe;
    }
    //! Gets a function always returning minimum identity
    /*! \return the function object; always safe */
    static integrity_fun min() {
        return integrity_fun{[](auto&&, auto&&, auto&&) {
            return I::min();
        }};
    }
    //! Gets an identity function
    /*! \return the function object */
    static integrity_fun identity() {
        return {};
    }
    //! Gets a function always returning the limit indentity
    /*! \return the function object; always safe */
    static integrity_fun max() {
        return integrity_fun{[](auto&&, auto&& limit, auto&&) {
            return limit;
        }};
    }
};

static_assert(integrity_function<integrity_fun<integrity_single, operation_base<impl::operation_base_dummy_id>>,
              integrity_single, operation_base<impl::operation_base_dummy_id>>);
static_assert(integrity_function<integrity_fun<integrity_single, operation_base<impl::operation_base_dummy_id>, false>,
              integrity_single, operation_base<impl::operation_base_dummy_id>>);
static_assert(integrity_function<integrity_fun<integrity_single, operation_base<impl::operation_base_dummy_id>, true>,
              integrity_single, operation_base<impl::operation_base_dummy_id>>);

//! A polymorphic function wrapper that satisfies concept integrity_function
/*! It differs from its base class std::function in that if it
 * contains no target, the call operator works as the identity function
 * (returns the input integrity) instead of throwing \c std::bad_function_call.
 * It is always safe, regardless of safety of the target function.
 * \tparam I an integrity type
 * \tparam O a SOFI operation type */
template <integrity I, operation O>
class safe_integrity_fun: std::function<I(const I&, const I&, const O&)> {
    //! The alias for the base class
    using base_t = std::function<I(const I&, const I&, const O&)>;
    using base_t::base_t;
public:
    //! Calls the stored target function
    /*! \param[in] i an itegrity value to be modified while passed from a
     * source to a destination object
     * \param[in] limit a hint for result, the should (but is not required to)
     * make its result less or equal to \a limit
     * \param[in] op the operation related to the call of this function
     * \return the result of the target function; <tt>i*limit</tt> if there is
     * no target function */
    I operator()(const I& i, const I& limit, const O& op) const {
        if (*this)
            return base_t::operator()(i, limit, op);
        else
            return i * limit;
    }
    //! Gets the safety status of this function
    /*! \return the safety status */
    [[nodiscard]] constexpr bool safe() const noexcept {
        return true;
    }
    //! Gets a function always returning minimum identity
    /*! \return the function object; always safe */
    static safe_integrity_fun min() {
        return safe_integrity_fun{[](auto&&, auto&&, auto&&) {
            return I::min();
        }};
    }
    //! Gets an identity function
    /*! \return the function object; always safe */
    static safe_integrity_fun identity() {
        return {};
    }
    //! Gets a function always returning the limit indentity
    /*! \return the function object; always safe */
    static safe_integrity_fun max() {
        return integrity_fun{[](auto&&, auto&& limit, auto&&) {
            return limit;
        }};
    }
};

static_assert(integrity_function<safe_integrity_fun<integrity_single, operation_base<impl::operation_base_dummy_id>>,
              integrity_single, operation_base<impl::operation_base_dummy_id>>);

//! Requirements for a class representing an entity
/*! An entity type must provide:
 * \arg type aliases \c integrity_t, \c operation_t, \c verdict_t, \c
 * access_ctrl_t, \c integrity_fun_t
 * \arg function \c integrity() returning the current integrity
 * \arg function \c min_integrity() returning the current minimum integrity
 * \arg function \c access_ctrl() returning the access controller of the entity
 * \arg functions test_fun(), \c prov_fun(), \c recv_fun() returning the
 * integrity testing, providing, and receiving functions
 * \arg function \c integrity() that sets the integrity of the entity
 * \arg function \c min_integrity() that sets the minimum integrity of the entity
 * \tparam T an entity type
 * \tparam I an integrity type
 * \tparam O an operation type
 * \tparam V a verdict type
 * \tparam AC an access controller type
 * \tparam F an integrity modification function type */
template <class T, class I, class O, class V, class AC, class F> concept entity =
    std::is_object_v<T> && integrity<I> && operation<O> && verdict<V> &&
    access_controller<AC, I, O, V> && integrity_function<F, I, O> &&
    std::same_as<typename T::integrity_t, I> &&
    std::same_as<typename T::operation_t, O> &&
    std::same_as<typename T::verdict_t, V> &&
    std::same_as<typename T::access_ctrl_t, AC> &&
    std::same_as<typename T::integrity_fun_t, F> &&
    requires (const T c_entity, T entity, I i) {
        { c_entity.integrity() } -> std::same_as<const I&>;
        { c_entity.min_integrity() } -> std::same_as<const I&>;
        { c_entity.access_ctrl() } -> std::same_as<const AC&>;
        { c_entity.test_fun() } -> std::same_as<const F&>;
        { c_entity.prov_fun() } -> std::same_as<const F&>;
        { c_entity.recv_fun() } -> std::same_as<const F&>;
        entity.integrity(i);
        entity.min_integrity(i);
    };

//! A straightforward class template that satisfies concept entity.
/*! \tparam I an integrity type
 * \tparam O an operation type
 * \tparam V a verdict type
 * \tparam AC an access controller type
 * \tparam F an integrity modification function type */
template <integrity I, operation O, verdict V, access_controller<I, O, V> AC, integrity_function<I, O> F>
requires std::default_initializable<AC> && std::default_initializable<F>
class basic_entity {
public:
    //! The integrity type
    using integrity_t = I;
    //! The operation type
    using operation_t = O;
    //! The verdict type
    using verdict_t = V;
    //! The access controller type
    using access_ctrl_t = AC;
    //! The integrity modification function type
    using integrity_fun_t = F;
    //! The default constructor
    /*! Creates the entity with \c I::min() current and minimum integrity,
     * default constructed access controller, identity function for integrity
     * testing, and minimum function for integrity providing and receiving. */
    basic_entity() = default;
    //! Gets the current integrity.
    /*! \return the integrity */
    [[nodiscard]] const I& integrity() const noexcept {
        return _integrity;
    }
    //! Gets the minimum integrity.
    /*! \return the integrity */
    [[nodiscard]] const I& min_integrity() const noexcept {
        return _min_integrity;
    }
    //! Gets the access controller.
    /*! \return the access controller */
    [[nodiscard]] const AC& access_ctrl() const noexcept {
        return _access_ctrl;
    }
    //! Gets the access controller.
    /*! \return the access controller */
    [[nodiscard]] AC& access_ctrl() noexcept {
        return _access_ctrl;
    }
    //! Gets the integrity testing function.
    /*! \return the function */
    [[nodiscard]] const F& test_fun() const noexcept {
        return _test_fun;
    }
    //! Gets the integrity testing function.
    /*! \return the function */
    [[nodiscard]] F& test_fun() noexcept {
        return _test_fun;
    }
    //! Gets the integrity providing function.
    /*! \return the function */
    [[nodiscard]] const F& prov_fun() const noexcept {
        return _prov_fun;
    }
    //! Gets the integrity providing function.
    /*! \return the function */
    [[nodiscard]] F& prov_fun() noexcept {
        return _prov_fun;
    }
    //! Gets the integrity receiving function.
    /*! \return the function */
    [[nodiscard]] const F& recv_fun() const noexcept {
        return _recv_fun;
    }
    //! Gets the integrity receiving function.
    /*! \return the function */
    [[nodiscard]] F& recv_fun() noexcept {
        return _recv_fun;
    }
    //! Sets the current integrity.
    /*! \param[in] val the new value */
    void integrity(integrity_t val) {
        _integrity = std::move(val);
    }
    //! Sets the minimum integrity.
    /*! \param[in] val the new value */
    void min_integrity(integrity_t val) {
        _min_integrity = std::move(val);
    }
    //! Sets the access controller.
    /*! \param[in] val the new value */
    void access_ctrl(AC val) {
        _access_ctrl = std::move(val);
    }
    //! Sets the integrity testing function.
    /*! \param[in] val the new value */
    void test_fun(F val) {
        _test_fun = std::move(val);
    }
    //! Sets the integrity providing function.
    /*! \param[in] val the new value */
    void prov_fun(F val) {
        _prov_fun = std::move(val);
    }
    //! Sets the integrity receiving function.
    /*! \param[in] val the new value */
    void recv_fun(F val) {
        _recv_fun = std::move(val);
    }
private:
    //! The current integrity
    integrity_t _integrity = integrity_t::min();
    //! The minimum integrity
    integrity_t _min_integrity = integrity_t::min();
    //! The access controller
    access_ctrl_t _access_ctrl{};
    //! The testing function
    integrity_fun_t _test_fun = I::identity();
    //! The integrity providing function
    integrity_fun_t _prov_fun = I::min();
    //! The integrity receiving function
    integrity_fun_t _recv_fun = I::min();
};

} // namespace soficpp
