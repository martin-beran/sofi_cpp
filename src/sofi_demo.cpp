/*! \file
 * \brief Program \c sofi_demo
 *
 * This is a simple demonstration and experimental program for the SOFI C++
 * library. It stores a persistent state of a SOFI system in a SQLite database.
 * The SOFI system is operated in several steps:
 * <ol>
 * <li>The database is initialized by <tt>sofi_demo init <em>file.db</em></tt>
 * <li>The database is populated with parts of a SOFI system: entities, access
 * controllers, integrity functions, etc. Any program can be used to manipulate
 * the database, for example, the standard command line client \c sqlite3.
 * <li>A list of intended SOFI operations is inserted into the database, using
 * any SQLite client program.
 * <li>The list of operations is executed by <tt>sofi_demo run
 * <em>file.db</em></tt>, which stores the results in the database.
 * <li>The results of operations can be examined by any SQLite client program.
 * </ol>
 *
 * The database schema is currently documented only by the initialization SQL
 * statements and comments in cmd_init().
 *
 * \test in file test_sofi_demo.cpp
 */

#include "soficpp/agent.hpp"
#include "soficpp/enum_str.hpp"
#include "soficpp/soficpp.hpp"
#include "sqlite_cpp.hpp"

#include <cassert>
#include <cstddef>
#include <deque>
#include <iostream>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <variant>

//! SOFI classes used by program \c sofi_demo
namespace demo {

//! Known operations
/*! Definitions of operations by related operation classes must kept in sync
 * with table \c operation in the database. */
enum class op_id {
    no_op, //!< Performs only SOFI check
    read, //!< Reads data of the object and sets them as data of the subject
    write, //!< Writes data of the subject to the object (replaces object data)
    read_append, //!< Reads data of the object and appends them to data of the subject
    write_append, //!< Appends data of the subject to data of the object
    write_arg, //!< Writes operation argument to the object (replaces object data)
    append_arg, //!< Appends operation argument to data of the object
    swap, //!< Exchanges data of the subject and of the object
    set_integrity, //!< Sets the integrity of the object
    set_min_integrity, //!< Sets the minimum integrity of the object
    clone, //!< Creates a copy of the object
    destroy, //!< Deletes the object
};

} // namespace demo

//! \cond
SOFICPP_IMPL_ENUM_STR_INIT(demo::op_id) {
    SOFICPP_IMPL_ENUM_STR_VAL(demo::op_id, no_op),
    SOFICPP_IMPL_ENUM_STR_VAL(demo::op_id, read),
    SOFICPP_IMPL_ENUM_STR_VAL(demo::op_id, write),
    SOFICPP_IMPL_ENUM_STR_VAL(demo::op_id, read_append),
    SOFICPP_IMPL_ENUM_STR_VAL(demo::op_id, write_append),
    SOFICPP_IMPL_ENUM_STR_VAL(demo::op_id, write_arg),
    SOFICPP_IMPL_ENUM_STR_VAL(demo::op_id, append_arg),
    SOFICPP_IMPL_ENUM_STR_VAL(demo::op_id, swap),
    SOFICPP_IMPL_ENUM_STR_VAL(demo::op_id, set_integrity),
    SOFICPP_IMPL_ENUM_STR_VAL(demo::op_id, set_min_integrity),
    SOFICPP_IMPL_ENUM_STR_VAL(demo::op_id, clone),
    SOFICPP_IMPL_ENUM_STR_VAL(demo::op_id, destroy),
};
//! \endcond

namespace demo {

class entity;

//! The integrity type
/*! An integrity is a set of strings. A set of all possible strings is
 * represented by soficpp::integrity_set::universe. */
using integrity = soficpp::integrity_set<std::string>;

//! The verdict type
class verdict: public soficpp::simple_verdict {
public:
    //! The default constructor, sets \ref error to \c false.
    verdict() = default;
    //! Indication of an operation failure caused by other reasons than SOFI
    bool error = false;
    //! Indication of an operation destroying its object
    bool destroy = false;
};

//! A base class for defining operations
class operation: public soficpp::operation_base<op_id> {
public:
    //! The type of map of operations
    using ops_map_t = std::map<op_id, std::unique_ptr<operation>>;
    [[nodiscard]] std::string_view name() const override {
        std::lock_guard lck(_mtx);
        if (_name.empty())
            _name = soficpp::enum2str(id());
        return _name;
    }
    //! Executes the operation and stores the result in the verdict object.
    /*! \param[in, out] subject the subject of the operation
     * \param[in, out] object the object of the operation
     * \param[in] arg an argument of the operation
     * \param[out] result the result object, the operation result is stored in
     * verdict::error. */
    void execute(entity& subject, entity& object, const::std::string& arg, verdict& result) const {
        _destroy_object = false;
        if (!do_exec(subject, object, arg))
            result.error = true;
        else
            if (_destroy_object)
                result.destroy = true;
    }
    //! Stores a database connection that can be used by do_exec() called in the current thread.
    /*! \param[in] db a database connection, or \c nullptr to unregisted any
     * stored database connection */
    static void attach_db(sqlite::connection* db = nullptr) {
        _db = db;
    }
    //! Gets the map of all operations.
    /*! \return the map containing all known operations */
    static const ops_map_t& get();
    //! Gets the object implementing a particular operation.
    /*! \param[in] id the id of the requests operation
     * \return the operation object
     * \throw std::invalid_argument if there is no operation object for a given
     * \a id */
    static const operation& get(op_id id);
protected:
    //! Implementation of the operation.
    /*! The default implementation does nothing and returns success.
     * \param[in, out] subject the subject of the operation
     * \param[in, out] object the object of the operation
     * \param[in] arg an argument of the operation
     * \return \c true if successful, \c false on failure */
    virtual bool do_exec([[maybe_unused]] entity& subject, [[maybe_unused]] entity& object,
                         [[maybe_unused]] const std::string& arg) const
    {
        return true;
    }
    //! Can be called by do_exec() to request destroying the object of the current operation in the current thread.
    static void destroy_object() {
        _destroy_object = true;
    }
    static thread_local sqlite::connection* _db; //!< A database connection that can be used by do_exec()
private:
    mutable std::mutex _mtx{}; //!< A mutex for _name
    mutable std::string _name{}; //!< Cached result of name()
    static thread_local bool _destroy_object; //!< Used by execute() and destroy_object()
};

thread_local sqlite::connection* operation::_db = nullptr;
thread_local bool operation::_destroy_object = false;

//! The access controller (ACL) type
using acl = soficpp::ops_acl<integrity, operation, verdict>;

//! The type for minimum integrity of an entity
using min_integrity = soficpp::acl<integrity, operation, verdict>;

//! The type an integrity modification function
class integrity_fun: public std::vector<std::pair<integrity, std::optional<integrity>>> {
    //! The base class
    using base_t = std::vector<std::pair<integrity, std::optional<integrity>>>;
    using base_t::base_t;
public:
    //! The integrity type
    using integrity_t = integrity;
    //! The operation type
    using operation_t = operation;
    //! Evaluates the integrity function
    /*! It tests \a i against all elements (pairs) in this vector. If \a i is
     * greater or equal to the first integrity in a pair, then then all
     * elements of the second integrity in the pair are added to the result. If
     * the second value in the pair is \c nullopt, then \a i is added to the
     * result.
     * \param[in] i an integrity
     * \param[in] limit a limit for the result
     * \param[in] op an operation (unused)
     * \return the modified integrity */
    integrity operator()(const integrity& i, const integrity& limit, const operation& op) const;
    //! Gets safety of the function
    /*! \return always \c true */
    [[nodiscard]] static constexpr bool safe() noexcept {
        return true;
    }
    //! Gets a function always returning minimum integrity
    /*! \return the function object */
    static integrity_fun min() {
        integrity_fun f{};
        f.comment = "min";
        return f;
    }
    //! Gets an identity function
    /*! \return the function object returning the argument \a i limited by \a
     * limit */
    static integrity_fun identity() {
        integrity_fun f{};
        f.comment = "identity";
        f.emplace_back(integrity{}, std::nullopt);
        return f;
    }
    //! Gets a function always returning the maximum indentity.
    /*! \return the function object */
    static integrity_fun max() {
        integrity_fun f{};
        f.comment = "max";
        f.emplace_back(integrity{}, integrity{integrity::universe{}});
        return f;
    }
    //! The comment of the integrity
    std::string comment;
};

integrity integrity_fun::operator()(const integrity& i, const integrity& limit, const operation&) const
{
    integrity result{};
    for (auto&& v: *this)
         if (v.first <= i) {
             if (v.second)
                 result = result + *v.second;
             else
                 result = result + i;
         }
    return result * limit;
}

//! The entity type
class entity: public soficpp::basic_entity<integrity, min_integrity, operation, verdict, acl, integrity_fun> {
public:
    //! The name of the entity, used as the primary key in the database
    std::string name{};
    //! Data of the entity, usable in operations
    std::string data{};
};

//! The agent class that exports to and imports from the database
class agent {
public:
    //! The entity type
    using entity_t = entity;
    //! The message type is the name of the entity in the database
    using message_t = std::string;
    //! Creates the agent.
    /*! \param[in] db a database connection used for export and import */
    explicit agent(sqlite::connection& db);
    //! The export operation
    /*! It saves the entity to the database.
     * \param[in] e an entity
     * \param[out] m a message
     * \return the result of export */
    soficpp::agent_result export_msg(const entity_t& e, message_t& m);
    //! The import operation
    /*! It reads the entity from the database.
     * \param[in] m a message (an entity name)
     * \param[out] e an entity
     * \return the result of import */
    soficpp::agent_result import_msg(const message_t& m, entity_t& e);
private:
    //! Thrown if something cannot be exported or imported
    struct export_import_error: public std::runtime_error {
        //! Creates the exception object.
        export_import_error(): runtime_error("export_import_error") {}
    };
    //! Exports an integrity to the database
    /*! \param[in] i an integrity to be exported
     * \return id of the exported integrity
     * \throw export_import_error if the integrity cannot be exported */
    int64_t export_msg_integrity(const integrity& i);
    //! Exports an ACL to the database
    /*! \param[in] a the ACL to be exported
     * \return id of the exported ACL
     * \throw export_import_error if the ACL cannot be exported */
    int64_t export_msg_acl(const acl& a);
    //! Exports an ACL to the database
    /*! \param[in] a the ACL to be exported
     * \param[in] op the operation controlled by ACL \a a
     * \param[in] id if it is not \c std::nullopt, then it is used as the id of the
     * ACL in the database; if \c std::nullopt, then a new id will be generated
     * \return id of the exported ACL
     * \throw export_import_error if the ACL cannot be exported */
    int64_t export_msg_acl(const acl::acl_t& a, std::optional<op_id> op = {}, std::optional<int64_t> id = {});
    //! Exports an integrity modification function to the database
    /*! \param[in] f a function to be exported
     * \return id of the exported function
     * \throw export_import_error if the function cannot be exported */
    int64_t export_msg_int_fun(const integrity_fun& f);
    //! Imports an integrity from the database by id
    /*! \param[in] id the id of the integrity
     * \return the imported integrity
     * \throw export_import_error if the integrity cannot be imported */
    integrity import_msg_integrity(int64_t id);
    //! Imports a minimum integrity from the database by id
    /*! \param[in] id the id of the minimum integrity
     * \return the imported integrity
     * \throw export_import_error if the minimum integrity cannot be imported */
    min_integrity import_msg_min_integrity(int64_t id);
    //! Imports an ACL from the database by id
    /*! \param[in] id the id of the ACL
     * \return the imported ACL
     * \throw export_import_error if the ACL cannot be imported */
    acl import_msg_acl(int64_t id);
    //! Imports an integrity modification function from the database by id
    /*! \param[in] id the id of the function
     * \return the imported function
     * \throw export_import_error if the function cannot be imported */
    integrity_fun import_msg_int_fun(int64_t id);
    sqlite::query qexp_entity; //!< SQL query for exporting an entity
    sqlite::query qexp_integrity_id; //!< SQL query for inserting into INTEGRITY_ID
    sqlite::query qexp_integrity; //!< SQL query for inserting into INTEGRITY
    sqlite::query qexp_acl_id; //!< SQL query for inserting into ACL_ID
    sqlite::query qexp_acl; //!< SQL query for inserting into ACL
    sqlite::query qexp_int_fun_id; //!< SQL query for inserting int INT_FUN_ID
    sqlite::query qexp_int_fun; //!< SQL query for inserting int INT_FUN
    sqlite::query qimp_entity; //!< SQL query for importing an entity
    sqlite::query qimp_integrity; //!< SQL query for importing an integrity
    sqlite::query qimp_min_integrity; //!< SQL query for importing a minimum integrity
    sqlite::query qimp_acl; //!< SQL query for importing an ACL
    sqlite::query qimp_int_fun; //!< SQL query for importing an integrity modification function
};

agent::agent(sqlite::connection& db):
    qexp_entity(db, R"(insert or replace into entity values ($1, $2, $3, $4, $t, $6, $7, $8))"),
    qexp_integrity_id(db, R"(insert into integrity_id select max(id) + 1, $1 from integrity_id returning id)"),
    qexp_integrity(db, R"(insert into integrity values ($1, $2))"),
    qexp_acl_id(db, R"(insert into acl_id select max(id) + 1 from acl_id returning id)"),
    qexp_acl(db, R"(insert into acl values ($1, $2, $3))"),
    qexp_int_fun_id(db, R"(insert into int_fun_id select max(id) + 1, $1 from int_fun_id returning id)"),
    qexp_int_fun(db, R"(insert into int_fun values ($1, $2, $3))"),
    qimp_entity(db, R"(
        select name, integrity, min_integrity, access_ctrl, test_fun, prov_fun, recv_fun, data
        from entity where name = $1)"),
    qimp_integrity(db, R"(select universe, elem from integrity_id left join integrity using (id) where id == $1)"),
    qimp_min_integrity(db, R"(select integrity from min_integrity where id = $1 and integrity is not null)"),
    qimp_acl(db, R"(select op, integrity from acl where id = $1)"),
    qimp_int_fun(db, R"(select comment, cmp, plus from int_fun_id left join int_fun using (id) where id = $1)")
{
}

soficpp::agent_result agent::export_msg(const entity_t& e, message_t& m)
{
    try {
        m = e.name;
        int64_t id = export_msg_integrity(e.integrity());
        int64_t min_id = export_msg_acl(e.min_integrity());
        int64_t access_ctrl = export_msg_acl(e.access_ctrl());
        int64_t test_fun = export_msg_int_fun(e.test_fun());
        int64_t prov_fun = export_msg_int_fun(e.prov_fun());
        int64_t recv_fun = export_msg_int_fun(e.recv_fun());
        qexp_entity.start().bind(1, e.name).bind(2, id).bind(3, min_id).bind(4, access_ctrl).
            bind(5, test_fun).bind(6, prov_fun).bind(7, recv_fun).bind(8, e.data).next_row();
    } catch (const sqlite::error& e) {
        std::cerr << e.what();
        return soficpp::agent_result{soficpp::agent_result::error};
    }
    return soficpp::agent_result{soficpp::agent_result::success};
}

int64_t agent::export_msg_acl(const acl& a)
{
    static acl::acl_t null_acl{};
    int64_t id = export_msg_acl(a.default_op ? *a.default_op : null_acl, std::nullopt, std::nullopt);
    for (auto&& o: a)
        export_msg_acl(o.second ? *o.second : null_acl, o.first, id);
    return id;
}

int64_t agent::export_msg_acl(const acl::acl_t& a, std::optional<op_id> op, std::optional<int64_t> id)
{
    if (!id) {
        auto ok = qexp_acl_id.start().next_row() == sqlite::query::status::row;
        assert(ok);
        assert(qexp_acl_id.column_count() == 1);
        if (auto v = qexp_acl_id.get_column(0); auto p = std::get_if<int64_t>(&v))
            id = *p;
        else
            throw export_import_error{};
    }
    assert(id);
    if (a.empty()) {
        qexp_acl.start().bind(1, *id);
        if (op)
            qexp_acl.bind(2, soficpp::enum2str(*op));
        else
            qexp_acl.bind(2, nullptr);
        qexp_acl.bind(3, nullptr).next_row();
    } else
        for (auto&& i: a) {
            int64_t integrity_id = export_msg_integrity(i);
            qexp_acl.start().bind(1, *id);
            if (op)
                qexp_acl.bind(2, soficpp::enum2str(*op));
            else
                qexp_acl.bind(2, nullptr);
            qexp_acl.bind(3, integrity_id).next_row();
        }
    return *id;
}

int64_t agent::export_msg_int_fun(const integrity_fun& f)
{
    auto ok = qexp_int_fun_id.start().bind(1, f.comment).next_row() == sqlite::query::status::row;
    assert(ok);
    assert(qexp_int_fun_id.column_count() == 1);
    int64_t id = 0;
    if (auto v = qexp_int_fun_id.get_column(0); auto p = std::get_if<int64_t>(&v))
        id = *p;
    else
        throw export_import_error{};
    for (auto&& v: f) {
        int64_t cmp = export_msg_integrity(v.first);
        qexp_int_fun.start().bind(1, id).bind(2, cmp);
        if (v.second) {
            int64_t plus = export_msg_integrity(*v.second);
            qexp_int_fun.bind(3, plus);
        } else
            qexp_int_fun.bind(3, nullptr);
        qexp_int_fun.next_row();
    }
    return id;
}

int64_t agent::export_msg_integrity(const integrity& i)
{
    bool universe = i == integrity{integrity::universe{}};
    auto ok = qexp_integrity_id.start().bind(1, universe).next_row() == sqlite::query::status::row;
    assert(ok);
    assert(qexp_integrity_id.column_count() == 1);
    int64_t id = 0;
    if (auto v = qexp_integrity_id.get_column(0); auto p = std::get_if<int64_t>(&v))
        id = *p;
    else
        throw export_import_error{};
    if (universe)
        return id;
    for (auto&& e: std::get<integrity::set_t>(i.value()))
        qexp_integrity.start().bind(1, id).bind(2, e).next_row();
    return id;
}

soficpp::agent_result agent::import_msg(const message_t& m, entity_t& e)
{
    try {
        qimp_entity.start().bind(1, m);
        if (qimp_entity.next_row() == sqlite::query::status::done)
            return soficpp::agent_result{soficpp::agent_result::error};
        assert(qimp_entity.column_count() == 8);
        if (auto v = qimp_entity.get_column(0); auto p = std::get_if<std::string>(&v))
            e.name = *p;
        else
            return soficpp::agent_result{soficpp::agent_result::error};
        if (auto v = qimp_entity.get_column(1); auto p = std::get_if<int64_t>(&v))
            e.integrity() = import_msg_integrity(*p);
        else
            return soficpp::agent_result{soficpp::agent_result::error};
        if (auto v = qimp_entity.get_column(2); auto p = std::get_if<int64_t>(&v))
            e.min_integrity() = import_msg_min_integrity(*p);
        else
            return soficpp::agent_result{soficpp::agent_result::error};
        if (auto v = qimp_entity.get_column(3); auto p = std::get_if<int64_t>(&v))
            e.access_ctrl() = import_msg_acl(*p);
        else
            return soficpp::agent_result{soficpp::agent_result::error};
        if (auto v = qimp_entity.get_column(4); auto p = std::get_if<int64_t>(&v))
            e.test_fun() = import_msg_int_fun(*p);
        else
            return soficpp::agent_result{soficpp::agent_result::error};
        if (auto v = qimp_entity.get_column(5); auto p = std::get_if<int64_t>(&v))
            e.prov_fun() = import_msg_int_fun(*p);
        else
            return soficpp::agent_result{soficpp::agent_result::error};
        if (auto v = qimp_entity.get_column(6); auto p = std::get_if<int64_t>(&v))
            e.recv_fun() = import_msg_int_fun(*p);
        else
            return soficpp::agent_result{soficpp::agent_result::error};
        if (auto v = qimp_entity.get_column(7); auto p = std::get_if<std::string>(&v))
            e.data = *p;
        else
            return soficpp::agent_result{soficpp::agent_result::error};
    } catch (const export_import_error&) {
        return soficpp::agent_result{soficpp::agent_result::error};
    } catch (const sqlite::error& e) {
        std::cerr << e.what();
        return soficpp::agent_result{soficpp::agent_result::error};
    }
    return soficpp::agent_result{soficpp::agent_result::success};
}

acl agent::import_msg_acl(int64_t id)
{
    qimp_acl.start().bind(1, id);
    acl result;
    while (qimp_acl.next_row() == sqlite::query::status::row) {
        assert(qimp_acl.column_count() == 2);
        auto& pacl = [&, this]() -> std::shared_ptr<acl::acl_t>& {
            auto v = qimp_acl.get_column(0);
            if (std::holds_alternative<std::nullptr_t>(v))
                return result.default_op;
            else if (auto p = std::get_if<std::string>(&v))
                try {
                    return result[soficpp::str2enum<op_id>(*p)];
                } catch (const std::invalid_argument&) {
                    throw export_import_error{};
                }
            else
                throw export_import_error{};
        }();
        if (!pacl)
            pacl = std::make_shared<acl::acl_t>();
        auto v = qimp_acl.get_column(1);
        if (std::holds_alternative<std::nullptr_t>(v))
            ;
        else if (auto p = std::get_if<int64_t>(&v))
            pacl->push_back(import_msg_integrity(*p));
        else
            throw export_import_error{};
    }
    return result;
}

integrity_fun agent::import_msg_int_fun(int64_t id)
{
    qimp_int_fun.start().bind(1, id);
    integrity_fun result;
    while (qimp_int_fun.next_row() == sqlite::query::status::row) {
        assert(qimp_int_fun.column_count() == 3);
        if (result.comment.empty()) {
            auto v = qimp_int_fun.get_column(0);
            if (std::holds_alternative<std::nullptr_t>(v))
                ;
            else if (auto p = std::get_if<std::string>(&v))
                result.comment = std::move(*p);
            else
                throw export_import_error{};
        }
        auto v = qimp_int_fun.get_column(1);
        if (std::holds_alternative<std::nullptr_t>(v))
            ;
        else if (auto pcmp = std::get_if<std::int64_t>(&v)) {
            v = qimp_int_fun.get_column(2);
            if (std::holds_alternative<std::nullptr_t>(v))
                result.emplace_back(import_msg_integrity(*pcmp), std::nullopt);
            else if (auto pplus = std::get_if<std::int64_t>(&v))
                result.emplace_back(import_msg_integrity(*pcmp), import_msg_integrity(*pplus));
            else
                throw export_import_error{};
        } else
            throw export_import_error{};
    }
    return result;
}

integrity agent::import_msg_integrity(int64_t id)
{
    qimp_integrity.start().bind(1, id);
    integrity::set_t result{};
    bool row = false;
    while (qimp_integrity.next_row() == sqlite::query::status::row) {
        assert(qimp_integrity.column_count() == 2);
        row = true;
        auto e = qimp_integrity.get_column(1);
        if (std::holds_alternative<std::nullptr_t>(e)) {
            auto u = qimp_integrity.get_column(0);
            if (auto p = std::get_if<int64_t>(&u)) {
                if (*p)
                    return integrity{integrity::universe{}};
                // not universe => empty set
            } else
                throw export_import_error{};
        } else if (auto p = std::get_if<std::string>(&e)) {
            result.insert(std::move(*p));
        } else
            throw export_import_error{};
    }
    if (!row)
        throw export_import_error{};
    return integrity{result};
}

min_integrity agent::import_msg_min_integrity(int64_t id)
{
    qimp_min_integrity.start().bind(1, id);
    min_integrity result{};
    while (qimp_min_integrity.next_row() == sqlite::query::status::row) {
        assert(qimp_min_integrity.column_count() ==  1);
        if (auto v = qimp_min_integrity.get_column(0); auto p = std::get_if<int64_t>(&v))
            result.push_back(import_msg_integrity(*p));
        else
            throw export_import_error{};
    }
    return result;
}

//! The implementation of op_id::no_op
class operation_no_op: public operation {
public:
    [[nodiscard]] id_t id() const override {
        return op_id::no_op;
    }
};

//! The implementation of op_id::read
class operation_read: public operation {
public:
    [[nodiscard]] bool is_read() const override {
        return true;
    }
    [[nodiscard]] id_t id() const override {
        return op_id::read;
    }
protected:
    bool do_exec(entity& subject, entity& object, const std::string&) const override {
        subject.data = object.data;
        return true;
    }
};

//! The implementation of op_id::write
class operation_write: public operation {
public:
    [[nodiscard]] bool is_write() const override {
        return true;
    }
    [[nodiscard]] id_t id() const override {
        return op_id::write;
    }
protected:
    bool do_exec(entity& subject, entity& object, const std::string&) const override {
        object.data = subject.data;
        return true;
    }
};

//! The implementation of op_id::read_append
class operation_read_append: public operation {
public:
    [[nodiscard]] bool is_read() const override {
        return true;
    }
    [[nodiscard]] id_t id() const override {
        return op_id::read_append;
    }
protected:
    bool do_exec(entity& subject, entity& object, const std::string&) const override {
        subject.data += object.data;
        return true;
    }
};

//! The implementation of op_id::write_append
class operation_write_append: public operation {
public:
    [[nodiscard]] bool is_write() const override {
        return true;
    }
    [[nodiscard]] id_t id() const override {
        return op_id::write_append;
    }
protected:
    bool do_exec(entity& subject, entity& object, const std::string&) const override {
        object.data += subject.data;
        return true;
    }
};

//! The implementation of op_id::write_arg
class operation_write_arg: public operation {
public:
    [[nodiscard]] bool is_write() const override {
        return true;
    }
    [[nodiscard]] id_t id() const override {
        return op_id::write_arg;
    }
protected:
    bool do_exec(entity&, entity& object, const std::string& arg) const override {
        object.data = arg;
        return true;
    }
};

//! The implementation of op_id::append_arg
class operation_append_arg: public operation {
public:
    [[nodiscard]] bool is_write() const override {
        return true;
    }
    [[nodiscard]] id_t id() const override {
        return op_id::append_arg;
    }
protected:
    bool do_exec(entity&, entity& object, const std::string& arg) const override {
        object.data += arg;
        return true;
    }
};

//! The implementation of op_id::swap
class operation_swap: public operation {
public:
    [[nodiscard]] bool is_read() const override {
        return true;
    }
    [[nodiscard]] bool is_write() const override {
        return true;
    }
    [[nodiscard]] id_t id() const override {
        return op_id::swap;
    }
protected:
    bool do_exec(entity& subject, entity& object, const std::string&) const override {
        using std::swap;
        swap(subject.data, object.data);
        return true;
    }
};

//! The implementation of op_id::set_integrity
/*! The new integrity is passed as the argument of the operation in the sane
 * JSON format as used by database view \c integrity_json: it is either the
 * string \c "universe", or a JSON array containing string elements. */
class operation_set_integrity: public operation {
public:
    [[nodiscard]] bool is_write() const override {
        return true;
    }
    [[nodiscard]] id_t id() const override {
        return op_id::set_integrity;
    }
    //! Parses an integrity from a string.
    /*! \param[in] s an integrity in JSON format
     * \return the integrity; \c std::nullopt if no database connection has
     * been attached to this thread by attach_db() or if \a s is invalid */
    static std::optional<integrity> str2integrity(const std::string& s) {
        integrity::set_t i{};
        if (!_db)
            goto error;
        for (auto q = std::move(sqlite::query{*_db,
                                R"(select key, value, type from json_each(?1))"}.start().bind(1, s));
            q.next_row() == sqlite::query::status::row;)
        {
            assert(q.column_count() == 3);
            auto type = q.get_column(2);
            if (auto t = std::get_if<std::string>(&type); !t || *t != "text")
                goto error;
            auto key = q.get_column(0);
            auto value = q.get_column(1);
            if (auto v = std::get_if<std::string>(&value)) {
                if (auto k = std::get_if<std::nullptr_t>(&key)) {
                    if (*v != "universe")
                        goto error;
                    else
                        return integrity{integrity::universe{}};
                } else
                    i.insert(*v);
            } else
                goto error;
        }
        return integrity{std::move(i)};
error:
        std::cerr << "Invalid integrity JSON value " << s << std::endl;
        return std::nullopt;
    }
protected:
    bool do_exec(entity&, entity& object, const std::string& arg) const override {
        if (auto i = str2integrity(arg)) {
            object.integrity(std::move(*i));
            return true;
        } else
            return false;
    }
};

//! The implementation of op_id::set_min_integrity
/*! The new minimum integrity is passed in the argument of the operation as a
 * JSON array of integrities, with each integrity in the format used by
 * operation_set_integrity. */
class operation_set_min_integrity: public operation {
public:
    [[nodiscard]] bool is_write() const override {
        return true;
    }
    [[nodiscard]] id_t id() const override {
        return op_id::set_min_integrity;
    }
    //! Parses a minimum integrity from a string.
    /*! \param[in] s a JSON array of integrities
     * \return the minimum integrity; \c std::nullopt if no database connection
     * has been attached to this thread by attach_db() or if \a s is invalid */
    static std::optional<min_integrity> str2min_integrity(const std::string& s) {
        min_integrity::container_t ic{};
        if (!_db)
            goto error;
        for (auto q = std::move(sqlite::query{*_db, R"(select value from json_each(?1))"}.start().bind(1, s));
            q.next_row() == sqlite::query::status::row;)
        {
            assert(q.column_count() == 1);
            auto value = q.get_column(0);
            if (auto v = std::get_if<std::string>(&value)) {
                if (auto i = operation_set_integrity::str2integrity(*v))
                    ic.push_back(std::move(*i));
                else
                    goto error;
            } else
                goto error;
        }
        return min_integrity{std::move(ic)};
error:
        std::cerr << "Invalid minimum integrity JSON value " << s << std::endl;
        return std::nullopt;
    }
protected:
    bool do_exec(entity&, entity& object, const std::string& arg) const override {
        if (auto i = str2min_integrity(arg)) {
            object.min_integrity() = std::move(*i);
            return true;
        } else
            return false;
    }
};

//! The implementation of op_id::clone
class operation_clone: public operation {
public:
    [[nodiscard]] id_t id() const override {
        return op_id::clone;
    }
protected:
    bool do_exec(entity&, entity& object, const std::string& arg) const override {
        if (!_db)
            return false;
        entity cloned = object;
        cloned.name = arg;
        agent a{*_db};
        std::string exported_clone;
        if (!a.export_msg(cloned, exported_clone)) {
            std::cerr << "Cannot export cloned object \"" << cloned.name << "\"" << std::endl;
            return EXIT_FAILURE;
        }
        assert(cloned.name == exported_clone);
        return true;
    }
};

//! The implementation of op_id::destroy
class operation_destroy: public operation {
public:
    [[nodiscard]] id_t id() const override {
        return op_id::destroy;
    }
protected:
    bool do_exec(entity&, entity&, const std::string&) const override {
        destroy_object();
        return true;
    }
};

auto operation::get() -> const ops_map_t&
{
    static auto ops_map = []() {
        std::map<op_id, std::unique_ptr<operation>> ops;
        ops[op_id::no_op] = std::make_unique<operation_no_op>();
        ops[op_id::read] = std::make_unique<operation_read>();
        ops[op_id::write] = std::make_unique<operation_write>();
        ops[op_id::read_append] = std::make_unique<operation_read_append>();
        ops[op_id::write_append] = std::make_unique<operation_write_append>();
        ops[op_id::write_arg] = std::make_unique<operation_write_arg>();
        ops[op_id::append_arg] = std::make_unique<operation_append_arg>();
        ops[op_id::swap] = std::make_unique<operation_swap>();
        ops[op_id::set_integrity] = std::make_unique<operation_set_integrity>();
        ops[op_id::set_min_integrity] = std::make_unique<operation_set_min_integrity>();
        ops[op_id::clone] = std::make_unique<operation_clone>();
        ops[op_id::destroy] = std::make_unique<operation_destroy>();
        return ops;
    }();
    return ops_map;
}

const operation& operation::get(op_id id)
{
    auto& ops = get();
    if (auto it = ops.find(id); it != ops.end() && it->second)
        return *it->second;
    throw std::invalid_argument("Unknown operation id " + soficpp::enum2str(id));
}

//! The engine class
using engine = soficpp::engine<entity>;

} // namespace demo

namespace {

//! A single operation record
/*! It combines a request read from the database and a result to be written to
 * the database. */
struct op_record {
    int64_t id = {}; //!< Request id
    std::string subject = {}; //!< Subject name
    std::string object = {}; //!< Object name
    const demo::operation* op = nullptr; //!< Operation definition
    std::string arg = {}; //!< Argument of the operation
    std::string comment = {}; //!< Comment of the operation
    bool allowed = false; //!< Result: whether the operation is allowed by SOFI
    bool access = false; //!< Result of the SOFI access test
    bool min = false; //!< Result of the SOFI minimum integrity test
    bool error = true; //!< Result: whether the operation failed for a non-SOFI reason
    bool destroy = false; //!< Result: whether the operation destroyed the object
};

//! Displays a short help
/*! \param[in] argv0 argument \c argv[0] from main()
 * \param[in] msg an error message
 * \return program exit code (indicates a failure) */
int usage(const char* argv0, std::string_view msg)
{
    std::cerr << msg << "\n\nusage:\n\n" << argv0 << R"( init FILE
    Initializes a new database FILE.

)" << argv0 << R"( run FILE
    Executes SOFI operations in database FILE.
)";
    return EXIT_FAILURE;
}

//! Initializes the database
/*! \param[in] file the database file name
 * \return program exit code */
int cmd_init(std::string_view file)
{
    sqlite::connection db{std::string{file}, true};
    // Set WAL mode (persistent)
    sqlite::query(db, R"(pragma journal_mode=wal)").start().next_row();
    // Check foreign key constrains, must be set for every connection outside of transactions
    sqlite::query(db, R"(pragma foreign_keys=true)").start().next_row();
    sqlite::transaction tr{db};
    for (const auto& sql: {
        // Stores IDs of integrity values. This table is needed in order to use
        // integrity IDs as a foreign key, because a foreign key must be the
        // primary key or have a unique index. If UNIVERSE is TRUE, then
        // elements with this ID in table INTEGRITY are ignored.
        R"(create table integrity_id (
                id integer primary key,
                universe int not null default false,
                constraint integrity_id_not_negative check (id >= 0),
                constraint universe_bool check (universe == false or universe == true)
            ))",
        // Table of integrity values. Rows with the same ID define a single
        // integrity. If there is no row in INTEGRITY for an ID from
        // INTEGRITY_ID, then the integrity is either the empty set (lattice
        // minimum) or the lattice maximum, depending on INTEGRITY_ID.UNIVERSE.
        R"(create table integrity (
                id int references integrity_id(id) on delete cascade on update cascade,
                elem text,
                primary key (id, elem),
                constraint integrity_elem_not_empty check (elem != '')
            ) without rowid, strict)",
        R"(create index integrity_idx_id on integrity (id))",
        // Insertable JSON view of integrity values stored in table INTEGRITY,
        // one row for each integrity, represented as an (possibly empty) array
        // of strings, or a single string "universe"
        R"(create view integrity_json(id, elems) as
            select
                id,
                case
                    when universe then json_quote('universe')
                    when exists (select * from integrity where id == iid.id) then
                        (select json_group_array(elem) from integrity where id == iid.id)
                    else json_array()
                end
            from integrity_id as iid)",
        R"(create trigger integrity_json_insert instead of insert on integrity_json
            begin
                insert into integrity_id values (new.id, new.elems == json_quote('universe')) on conflict do nothing;
                insert into integrity select new.id, e.value from json_each(new.elems) as e where key is not null;
            end)",
        // Insert minimum and maximum integrity
        R"(insert into integrity_json values (0, '[]'), (1, '"universe"'))",
        // Table of operation definitions, identified by operation NAME. It
        // must be kept in sync with enum demo::op_id and with related
        // operation classes.
        R"(create table operation (
                name text primary key, is_read int not null, is_write int not null,
                rw_type text generated always as (
                    case
                        when not is_read and not is_write then 'no-flow'
                        when is_read and not is_write then 'read'
                        when not is_read and is_write then 'write'
                        when is_read and is_write then 'read-write'
                    end
                ) stored,
                constraint op_name_not_empty check (name != ''),
                constraint is_read_bool check (is_read == false or is_read == true),
                constraint is_write_bool check (is_write == false or is_write == true)
            ) without rowid, strict)",
        // Table of IDs of ACL values. This table is needed in order to use
        // ACL IDs as a foreign key, because a foreign key must be the
        // primary key or have a unique index.
        R"(create table acl_id (
                id integer primary key,
                constraint acl_id_not_negative check (id >= 0)
            ))",
        // Table of ACLs. Rows with the same ID define a single ACL with
        // semantics of soficpp::ops_acl containing soficpp::acl. That is,
        // there is an entry for each operation OP, and a default entry (with
        // NULL OP) controlling operations without specific entries. Each entry
        // is a (possibly empty, represented by NULL) set of integrities INT_ID. 
        R"(create table acl (
                id int not null references acl_id(id) on delete cascade on update cascade,
                op text references operation(name) on delete restrict on update restrict,
                integrity int references integrity_id(id) on delete restrict on update restrict,
                unique (id, op, integrity)
            ) strict)",
        R"(create index acl_idx_id on acl (id))",
        R"(create index acl_idx_op on acl (op))",
        R"(create index acl_idx_integrity on acl (integrity))",
        // Insertable view of table ACL that automatically adds missing ACL
        // IDs to table ACL_ID
        R"(create view acl_ins as select * from acl)",
        R"(create trigger acl_ins_insert instead of insert on acl_ins
            begin
                insert into acl_id values (new.id) on conflict do nothing;
                insert into acl values (new.id, new.op, new.integrity);
            end)",
        // Read-only view of ACLs that displays integrities in JSON format
        R"(create view acl_json(id, op, integrity) as
            select acl.id as id , acl.op as op, integrity_json.elems as integrity
            from acl left join integrity_json on acl.integrity = integrity_json.id
            order by id, op)",
        // Insert ACLs that deny all operations and allow all operations
        R"(insert into acl_ins values (0, null, null), (1, null, 0))",
        // Read-only view of ACLs that selects values usable as minimum integrity
        R"(create view min_integrity as select id, integrity from acl where op is null)",
        // JSON value of MIN_INTEGRITY
        R"(create view min_integrity_json as select id, integrity from acl_json where op is null)",
        // Table of IDs of INT_FUN values. This table is needed in order to use
        // integrity function IDs as a foreign key, because a foreign key must
        // be the primary key or have a unique index.
        R"(create table int_fun_id (
                id integer primary key,
                comment text default '',
                constraint int_fun_id_not_negative check (id >= 0)
            ))",
        // Table of integrity modification functions, usable as test, providing,
        // and receiving functions of entities. Each function is a set of pairs
        // of integrities. When evaluating a function, the integrity passed as
        // the argument is compared to the first integrity (CMP) in each pair.
        // If the argument is greater or equal, then all elements of the second
        // integrity (PLUS) in the pair is added to the function result. If the
        // second integrity of a pair is NULL, then all elements of the
        // argument are added to the result.
        R"(create table int_fun (
                id int not null references int_fun_id(id) on delete cascade on update cascade,
                cmp int not null references integrity_id(id) on delete restrict on update restrict,
                plus int references integrity_id(id) on delete restrict on update restrict
            ) strict)",
        R"(create index int_fun_idx_id on int_fun (id))",
        R"(create index int_fun_idx_cmp on int_fun (cmp))",
        R"(create index int_fun_idx_plus on int_fun (plus))",
        // Insertable view of table INT_FUN that automatically adds missing
        // function IDs to table INT_FUN_ID
        R"(create view int_fun_ins as
            select fi.id as id, f.cmp as cmp, f.plus as plus, fi.comment as comment
            from int_fun_id as fi join int_fun as f using (id))",
        R"(create trigger int_fun_ins_insert instead of insert on int_fun_ins
            begin
                insert into int_fun_id values (new.id, new.comment) on conflict do nothing;
                insert into int_fun values (new.id, new.cmp, new.plus);
            end)",
        // Read-only view of int_fun that displays integrities in JSON format
        R"(create view int_fun_json(id, cmp, plus, comment) as
            select f.id, c.elems, a.elems, fi.comment
            from
                int_fun_id as fi
                join int_fun as f using (id)
                left join integrity_json as c on f.cmp == c.id
                left join integrity_json as a on f.plus == a.id
            order by f.id)",
        // Insert a minimum integrity, identity, and maximum integrity functions
        R"(insert into int_fun_ins values (0, 0, 0, 'min'), (1, 0, null, 'identity'), (2, 0, 1, 'max'))",
        // Table of entities. DATA can be used (read and written) by implementations
        R"(create table entity (
                name text primary key,
                integrity int not null references integrity_id(id) on delete restrict on update restrict,
                min_integrity int not null references acl_id(id) on delete restrict on update restrict,
                access_ctrl int not null references acl_id(id) on delete restrict on update restrict,
                test_fun int not null references int_fun_id(id) on delete restrict on update restrict,
                prov_fun int not null references int_fun_id(id) on delete restrict on update restrict,
                recv_fun int not null references int_fun_id(id) on delete restrict on update restrict,
                data text default null
            ) without rowid, strict)",
        R"(create index entity_idx_integrity on entity (integrity))",
        R"(create index entity_idx_min_integrity on entity (min_integrity))",
        R"(create index entity_idx_access_ctrl on entity (access_ctrl))",
        R"(create index entity_idx_test_fun on entity (test_fun))",
        R"(create index entity_idx_prov_fun on entity (prov_fun))",
        R"(create index entity_idx_recv_fun on entity (recv_fun))",
        // Table of requested operations. Order of operations is defined by
        // ascending order of IDs. SUBJECT and OBJECT do not use foreign key
        // constraints referencing ENTITY.NAME, because the referenced entities
        // can be dynamically created and deleted by other operations. ARG is
        // passed as an argument to the implementation of an operation.
        R"(create table request (
                id integer primary key,
                subject text not null,
                object text not null,
                op text not null references operation(name) on delete restrict on update restrict,
                arg text default null,
                comment text default ''
            ) strict)",
        R"(create index request_idx_op on request (op))",
        // Table of operation results. Completed operations are moved from
        // REQUEST to RESULT. Columns shared with REQUEST are simply copied.
        // ALLOWED is the SOFI result of the operation. ACCESS is the result of
        // the access test, MIN is the result of the minimum integrity test.
        // ERROR indicates an operation failed for other reasons than being
        // denied by SOFI.
        R"(create table result (
                id integer,
                subject text not null,
                object text not null,
                op text not null references operation(name) on delete restrict on update restrict,
                arg text default null,
                comment text default '',
                allowed int not null,
                access int not null,
                min int not null,
                error int not null default false,
                constraint allowed_bool check (allowed == false or allowed == true),
                constraint access_bool check (access == false or access == true),
                constraint min_bool check (min == false or min == true),
                constraint error_bool check (error == false or error == true)
            ) strict)",
        R"(create index result_idx_op on result (op))",
    }) {
        sqlite::query(db, sql).start().next_row();
    }
    // Insert all known operations to table OPERATION
    for (auto&& [k, v]: demo::operation::get()) {
        sqlite::query{db, R"(insert into operation values (?1, ?2, ?3))"}.start().
            bind(1, soficpp::enum2str(k)).bind(2, v->is_read()).bind(3, v->is_write()).
            next_row();
    }
    tr.commit();
    return EXIT_SUCCESS;
}

//! Gets all operation requests from the database.
/*! \param[in] db a database connection
 * \return the operation requests */
std::deque<op_record> get_op_requests(sqlite::connection& db)
{
    std::deque<op_record> ops;
    for (auto q = std::move(sqlite::query{db,
                            R"(select id, subject, object, op, arg, comment from request order by id)"}.start());
         q.next_row() == sqlite::query::status::row;)
    {
        assert(q.column_count() != 6);
        op_record op{};
        auto get_val = [&q]<class T>(int i, T& v, bool null = false) {
            auto c = q.get_column(i);
            if (null && std::get_if<std::nullptr_t>(&c))
                return;
            if (auto p = std::get_if<T>(&c))
                v = *p;
            else
                throw std::runtime_error("Unexpected type of table REQUEST column " + std::to_string(i));
        };
        get_val(0, op.id);
        get_val(1, op.subject);
        get_val(2, op.object);
        std::string op_op;
        get_val(3, op_op);
        try {
            op.op = &demo::operation::get(soficpp::str2enum<demo::op_id>(op_op));
        } catch (const std::invalid_argument&) {
            throw std::runtime_error("Unknown operation name \"" + op_op + "\" in table REQUEST");
        };
        get_val(4, op.arg, true);
        get_val(5, op.comment, true);
        ops.push_back(std::move(op));
    }
    return ops;
}

//! Executes SOFI operation in a database
/*! \param[in] file the database file name
 * \return program exit code */
int cmd_run(std::string_view file)
{
    sqlite::connection db{std::string{file}, false};
    // Check foreign key constrains, must be set for every connection outside of transactions
    sqlite::query(db, R"(pragma foreign_keys=1)").start().next_row();
    // Read all operation requests
    std::deque<op_record> ops = get_op_requests(db);
    // Execute operations
    demo::engine engine{};
    demo::agent agent{db};
    sqlite::query sql_del_request{db, R"(delete from request where id = ?1)"};
    sqlite::query sql_ins_result{db, R"(insert into result values (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10))"};
    sqlite::query sql_del_entity{db, R"(delete from entity where name = ?1)"};
    for (auto& o: ops) {
        std::cout << "BEGIN " << o.id << ": " << o.comment << std::endl;
        sqlite::transaction tr{db};
        sql_del_request.start().bind(1, o.id).next_row();
        demo::entity subject;
        if (!agent.import_msg(o.subject, subject)) {
            std::cerr << "Cannot import subject \"" << o.subject << "\"" << std::endl;
            return EXIT_FAILURE;
        }
        assert(o.subject == subject.name);
        demo::entity object;
        if (!agent.import_msg(o.object, object)) {
            std::cerr << "Cannot import object \"" << o.object << "\"" << std::endl;
            return EXIT_FAILURE;
        }
        assert(o.object == object.name);
        assert(o.op);
        demo::verdict verdict = engine.operation(subject, object, *o.op);
        if (verdict) {
            demo::operation::attach_db(&db);
            o.op->execute(subject, object, o.arg, verdict);
            demo::operation::attach_db();
        }
        o.allowed = verdict.allowed();
        o.access = verdict.access_test();
        o.min = verdict.min_test();
        o.error = verdict.error;
        std::string exported_subject{};
        std::string exported_object{};
        if (!agent.export_msg(subject, exported_subject)) {
            std::cerr << "Cannot export subject \"" << subject.name << "\"" << std::endl;
            return EXIT_FAILURE;
        }
        assert(subject.name == exported_subject);
        if (o.destroy)
            sql_del_entity.start().bind(1, object.name).next_row();
        else {
            if (!agent.export_msg(object, exported_object)) {
                std::cerr << "Cannot export object \"" << object.name << "\"" << std::endl;
                return EXIT_FAILURE;
            }
            assert(object.name == exported_object);
        }
        sql_ins_result.start().
            bind(1, o.id).bind(2, o.subject).bind(3, o.object).bind(4, o.op->name()).bind(5, o.arg).bind(6, o.comment).
            bind(7, o.allowed).bind(8, o.access).bind(9, o.min).bind(10, o.error).
            next_row();
        tr.commit();
        std::cout << "END   " << o.id << " allowed=" << o.allowed <<
            " access=" << o.access << " min=" << o.min << " error=" << o.error << " destroy=" << o.destroy << std::endl;
    }
    return EXIT_SUCCESS;
}

} // namespace

//! The main function of program \c sofi_demo
/*! \param[in] argc the number of command line arguments
 * \param[in] argv the command line arguments
 * \return the program exit code, 0 for success, nonzero for failure */
int main(int argc, char* argv[])
{
    using namespace std::string_literals;
    using namespace std::string_view_literals;
    if (argc != 3)
        return usage(argv[0], "Invalid command line arguments");
    try {
        if (argv[1] == "init"sv)
            return cmd_init(argv[2]);
        if (argv[1] == "run"sv)
            return cmd_run(argv[2]);
        else
            return usage(argv[0], "Unknown command \""s + argv[1] + "\"");
    } catch (const sqlite::error& e) {
        std::cerr << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unhandled unknown exception" << std::endl;
    }
    return EXIT_FAILURE;
}
