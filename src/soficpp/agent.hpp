#pragma once

/*! \file
 * \brief DiSOFI -- distributed DiSOFI
 *
 * \test in file test_agent.cpp
 */

#include "entity.hpp"

namespace soficpp {

//! Requirements for a class representing a message
/*! There are no constraints on a message type. */
template <class T> concept message = true;

//! A result of an agent export/import operation
struct agent_result {
    //! The possible results
    enum result {
        //! Export or import successful
        success,
        //! Export or import failed
        error,
        //! The remote engine is not trusted for export, or the message is not authenticated for import
        /*! When exporting, the remote SOFI engine is not trusted, therefore it
         * is not allowed to get the exported entity. When importing, the
         * authenticity test of the message has failed, therefore the message
         * is not trusted for import. */
        untrusted,
    };
    //! Stores a result code.
    /*! \param[in] code the result code */
    explicit constexpr agent_result(result code = success) noexcept: code(code) {}
    //! Checks if the result represents success.
    /*! \return \c true if \ref code is \ref agent_result::success, \c false
     * otherwise */
    [[nodiscard]] bool ok() const noexcept {
        return code == success;
    }
    //! The same as ok()
    /*! \return the result of ok() */
    explicit operator bool() const noexcept {
        return ok();
    }
    //! The stored result code
    result code;
};

//! Requirements for a class representing an agent
/*! The agent represents an interface to a remote SOFI engine. It must provide:
 * \arg type aliases \c entity_t, \c message_t
 * \arg member function \c export_msg() for exporting an entity in the form of
 * a message that can be passed to the remote SOFI engine represented by the
 * agent
 * \arg memger function \c import_msg() for importing an entity from a message
 * received from the remote SOFI engine represented by the agent
 * \tparam T an agent type */
template <class T> concept agent =
    entity<typename T::entity_t> &&
    message<typename T::message_t> &&
    requires (T a, const typename T::entity_t e, typename T::message_t m) {
        { a.export_msg(e, m) } -> std::same_as<agent_result>;
    } &&
    requires (T a, const typename T::message_t m, typename T::entity_t e) {
        { a.import_msg(m, e) } -> std::same_as<agent_result>;
    };

//! A simple agent that exports and imports by copy between an entity and a message
/*! It satisfies concept soficpp::copy_agent.
 * The entity and the message are of the same type and a it must be copy
 * assignable. The result of an export or import can be controlled by setting
 * the appropriate member of type agent_result prior to the operation.
 * \tparam T a type of both entity and message
 * \test in file test_agent.cpp */
template <entity T> requires std::is_copy_assignable_v<T> class copy_agent {
public:
    //! The entity type
    using entity_t = T;
    //! The message type is the same as the entity
    using message_t = T;
    //! The export operation
    /*! It copies an entity to a message if export_result is
     * agent_result::success.
     * \param[in] e an entity
     * \param[out] m a message
     * \return the result of export, as defined by export_result */
    agent_result export_msg(const T& e, T& m) {
        if (export_result)
            m = e;
        return export_result;
    }
    //! The import operation
    /*! It copies an entity from a message if export_result is
     * agent_result::success.
     * \param[in] m a message
     * \param[out] e an entity
     * \return the result of import, as defined by import_result */
    agent_result import_msg(const T& m, T& e) {
        if (import_result)
            e = m;
        return import_result;
    }
    //! Result that will be returned by export_msg()
    agent_result export_result{agent_result::success};
    //! Result that will be returned by import_msg()
    agent_result import_result{agent_result::success};
};

static_assert(agent<copy_agent<basic_entity<
    integrity_single,
    acl_single<integrity_single, operation_base<impl::operation_base_dummy_id>, simple_verdict>,
    operation_base<impl::operation_base_dummy_id>,
    simple_verdict,
    ops_acl<integrity_single, operation_base<impl::operation_base_dummy_id>, simple_verdict>,
    integrity_fun<integrity_single, operation_base<impl::operation_base_dummy_id>>
>>>);

} // namespace soficpp
