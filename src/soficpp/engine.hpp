#pragma once

/*! \file
 * \brief SOFI engine -- operations and enforcing SOFI rules
 */

#include "entity.hpp"

#include <optional>

namespace soficpp {

//! The SOFI engine -- the main class enforcing SOFI rules
/*! It is a polymorphic class with empty virtual member functions called
 * between stages of operation processing, in order to be able, for example, to
 * customize storing additional information to a verdict, or to perform
 * logging. The main member function for SOFI operations is operation(). There
 * are other public member functions called by operation() that provide access
 * to parts of operation processing.
 * \tparam E an entity type */
template <entity E> class engine {
public:
    //! The entity type
    using entity_t = E;
    //! The integrity type
    using integrity_t = typename E::integrity_t;
    //! The operation type
    using operation_t = typename E::operation_t;
    //! The verdict type
    using verdict_t = typename E::verdict_t;
    //! The access controller type
    using access_ctrl_t = typename E::access_ctrl_t;
    //! The integrity modification function type
    using integrity_fun_t = typename E::integrity_fun_t;
    //! The default constructor
    engine() = default;
    //! No copy (is polymorphic)
    engine(const engine&) = delete;
    //! No move (is polymorphic)
    engine(engine&&) = delete;
    //! No copy (is polymorphic)
    engine& operator=(const engine&) = delete;
    //! No move (is polymorphic)
    engine& operator=(engine&&) = delete;
    //! Virtual destructor
    virtual ~engine() = default;
    //! Performs or tests a SOFI operation
    /*! It either performs the operation, including modifying subject and
     * object integrity, or it just tests if the operation is allowed, without
     * really executing it. The return value contains the new integrity values
     * only if \a execute is \c false. It \a execute is \c true, then both
     * integrity values in the result are \c std::nullopt. Arguments \a subj
     * and \a obj can be modified only if the operation is actually executed,
     * that is \a execute is \c true and the operation is allowed.
     * \param[in, out] subj the subject of the operation
     * \param[in, out] obj the object of the operation
     * \param[in] op the operation
     * \param[in] execute if \c true, the operation is performed; if \c false,
     * it is only tested if the operation can be performed
     * \return the verdict */
    verdict_t operation(entity_t& subj, entity_t& obj, const operation_t& op, bool execute = true) {
        auto [verdict, allow] = test_access(subj, obj, op, execute);
        if (!allow)
            return verdict;
        std::optional<integrity_t> i_subj;
        std::optional<integrity_t> i_obj;
        if (op.is_write())
            i_obj = pass_integrity(subj, obj, op);
        if (op.is_read())
            i_subj = pass_integrity(obj, subj, op);
        if (!test_min_integrity(subj, obj, op, execute, verdict, i_subj, i_obj))
            return verdict;
        if (execute) {
            if (i_subj)
                subj.integrity(std::move(*i_subj));
            if (i_obj)
                obj.integrity(std::move(*i_obj));
            execute_op(subj, obj, op, verdict);
        }
        return verdict;
    }
    //! Tests if an operation is allowed by the access controller.
    /*! It tests if operation \a op is allowed by object \a obj
     * access controller for subject \a subj. It creates the verdict object,
     * initializes it by init_verdict(), and stores the result of the test in
     * the verdict by calling its member function \c access_test().
     * \param[in] subj the subject of the operation
     * \param[in] obj the object of the operation
     * \param[in] op the operation
     * \param[in] execute if \c true, the operation is performed; if \c false,
     * it is only tested if the operation can be performed
     * \return a pair of the verdict object containing the access controller
     * test result and the indicator if the operation is allowed by the access
     * controller. */
    std::pair<verdict_t, bool> test_access(const entity_t& subj, const entity_t& obj, const operation_t& op,
                                           bool execute)
    {
        verdict_t verdict{};
        init_verdict(subj, obj, op, execute, verdict);
        bool allow = access_test(obj.access_ctrl().test(subj, op, verdict));
        verdict.access_test(allow);
        after_test_access(subj, obj, op, execute, verdict, allow);
        return {verdict, allow};
    }
    //! Computes a new integrity of a reader object according to the integrity of both the reader and the writer.
    /*! \param[in] writer a writer entity of an operation
     * \param[in] reader a reader entity of an operation
     * \param[in] op an operation
     * \return the new integrity of the reader object */
    integrity_t pass_integrity(const entity_t& writer, const entity_t& reader, const operation_t& op) {
        auto i_test = reader.test_fun()(writer.integrity(), reader.integrity(), op);
        if (!reader.test_fun().safe())
            i_test = i_test * reader.integrity();
        if (auto i_prov = writer.prov_fun()(writer.integrity(), writer.integrity(), op); i_prov != integrity_t::min()) {
            if (!writer.test_fun().safe())
                i_prov = i_prov * writer.integrity();
            if (auto i_recv = reader.recv_fun()(i_prov, i_prov, op); i_recv != integrity_t::min()) {
                if (!reader.recv_fun().safe())
                    i_recv = i_recv * i_prov;
                i_test = i_test + i_recv;
            }
        }
        return i_test;
    }
    //! Tests that an operation does not decrease the integrity of an entity below the allowed minimum.
    /*! \param[in] subj the subject of the operation
     * \param[in] obj the object of the operation
     * \param[in] op the operation
     * \param[in] execute if \c true, the operation is performed; if \c false,
     * it is only tested if the operation can be performed
     * \param[in, out] v the verdict object
     * \param[in] i_subj the new integrity of the subject, or std::nullopt if
     * \a op is not a read operation
     * \param[in] i_obj the new integrity of the object, or std::nullopt if \a
     * op it not a write operation
     * \return if both subject and object integrities may be adjusted according
     * to \a i_subj and \a i_obj */
    bool test_min_integrity(const entity_t& subj, const entity_t& obj, const operation_t& op,
                            bool execute, verdict_t& v,
                            const std::optional<integrity_t>& i_subj, const std::optional<integrity_t>& i_obj)
    {
        bool allow_min_subj = true;
        bool allow_min_obj = true;
        if (i_obj()) {
            allow_min_obj = *i_obj >= obj.min_integrity();
        }
        if (i_subj()) {
            allow_min_subj = *i_subj >= subj.min_integrity();
        }
        v.min_test(allow_min_subj && allow_min_obj);
        after_test_min(subj, obj, op, execute, v, i_subj, allow_min_subj, i_obj, allow_min_obj);
        return allow_min_subj && allow_min_obj;
    }
protected:
    //! Initializes a verdict.
    /*! The default implementation does nothing.
     * \param[in] subj the subject of the operation
     * \param[in] obj the object of the operation
     * \param[in] op the operation
     * \param[in] execute if \c true, the operation is performed; if \c false,
     * it is only tested if the operation can be performed
     * \param[in, out] v a newly created verdict object */
    virtual void init_verdict([[maybe_unused]] const entity_t& subj, [[maybe_unused]] const entity_t& obj,
                              [[maybe_unused]] const operation_t& op, [[maybe_unused]] bool execute,
                              [[maybe_unused]] verdict_t& v)
    {}
    //! Called after an access controller is tested and the result is stored in the verdict.
    /*! The default implementation does nothing.
     * \param[in] subj the subject of the operation
     * \param[in] obj the object of the operation
     * \param[in] op the operation
     * \param[in] execute if \c true, the operation is performed; if \c false,
     * it is only tested if the operation can be performed
     * \param[in, out] v the verdict object
     * \param[in] allow whether the operation is allowed by the access
     * controller */
    virtual void after_test_access([[maybe_unused]] const entity_t& subj, [[maybe_unused]] const entity_t& obj,
                                   [[maybe_unused]] const operation_t& op, [[maybe_unused]] bool execute,
                                   [[maybe_unused]] verdict_t& v, [[maybe_unused]] bool allow)
    {}
    //! Called when a an operation is to be executed.
    /*! It is called by operation() just before return, after the integrities
     * of the subject and object are adjusted, when the operation is to be
     * executed, that is, operation() is called with \c true in \a execute and
     * the operation is allowed.
     * \param[in, out] subj the subject of the operation
     * \param[in, out] obj the object of the operation
     * \param[in] op the operation
     * \param[in, out] v the verdict object */
    virtual void execute_op([[maybe_unused]] entity_t& subj, [[maybe_unused]] entity_t& obj,
                            [[maybe_unused]] const operation_t& op, [[maybe_unused]] verdict_t& v)
    {}
    //! Called after the minimum integrities are tested and the result is stored in the verdict.
    /*! The default implementation does nothing.
     * \param[in] subj the subject of the operation
     * \param[in] obj the object of the operation
     * \param[in] op the operation
     * \param[in] execute if \c true, the operation is performed; if \c false,
     * it is only tested if the operation can be performed
     * \param[in, out] v the verdict object
     * \param[in] i_subj the new integrity of the subject, or std::nullopt if
     * \a op is not a read operation
     * \param[in] allow_min_subj if \a i_subj is std::nullopt or allowed by the
     * minimum integrity of \a subj
     * \param[in] i_obj the new integrity of the object, or std::nullopt if \a
     * op it not a write operation
     * \param[in] allow_min_obj if \a i_obj is std::nullopt or allowed by the
     * minimum integrity of \a obj */
    virtual void after_test_min([[maybe_unused]] const entity_t& subj, [[maybe_unused]] const entity_t& obj,
                                [[maybe_unused]] const operation_t& op, [[maybe_unused]] bool execute,
                                [[maybe_unused]] verdict_t& v,
                                [[maybe_unused]] const std::optional<integrity_t>& i_subj,
                                [[maybe_unused]] bool allow_min_subj,
                                [[maybe_unused]] const std::optional<integrity_t>& i_obj,
                                [[maybe_unused]] bool allow_min_obj)
    {}
};

} // namespace soficpp
