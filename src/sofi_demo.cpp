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
 */

#include "soficpp/enum_str.hpp"
#include "soficpp/soficpp.hpp"
#include "sqlite_cpp.hpp"

#include <cstddef>
#include <deque>
#include <iostream>
#include <stdexcept>

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
};

//! A base class for defining operations
class operation: public soficpp::operation_base<op_id> {
public:
    //! The type of map of operations
    using ops_map_t = std::map<op_id, std::unique_ptr<operation>>;
    [[nodiscard]] std::string_view name() const override {
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
        if (!do_exec(subject, object, arg))
            result.error = true;
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
private:
    mutable std::string _name; //!< Cached result of name()
};

//! The access controller (ACL) type
using acl = soficpp::ops_acl<integrity, operation, verdict>;

//! The type for minimum integrity of an entity
using min_integrity = soficpp::acl_single<integrity, operation, verdict>;

//! The type an integrity modification function
using integrity_fun = soficpp::safe_integrity_fun<integrity, operation>;

class entity: public soficpp::basic_entity<integrity, min_integrity, operation, verdict, acl, integrity_fun> {
public:
    //! Data of the entity, usable in operations
    std::string data{};
};

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
class operation_set_integrity: public operation {
public:
    [[nodiscard]] bool is_write() const override {
        return true;
    }
    [[nodiscard]] id_t id() const override {
        return op_id::set_integrity;
    }
protected:
    bool do_exec(entity&, entity&, const std::string&) const override {
        // TODO
        return false;
    }
};

//! The implementation of op_id::set_min_integrity
class operation_set_min_integrity: public operation {
public:
    [[nodiscard]] bool is_write() const override {
        return true;
    }
    [[nodiscard]] id_t id() const override {
        return op_id::set_min_integrity;
    }
protected:
    bool do_exec(entity&, entity&, const std::string&) const override {
        // TODO
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
    bool do_exec(entity&, entity&, const std::string&) const override {
        // TODO
        return false;
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
        // TODO
        return false;
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
    throw std::invalid_argument("Unknown operation name " + soficpp::enum2str(id));
}

//! The agent class that exports to and imports from the database
class agent {
};

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
                id int references integrity_id(id) on delete restrict on update restrict,
                elem text,
                primary key (id, elem),
                constraint integrity_elem_not_empty check (elem != '')
            ) without rowid, strict)",
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
                insert into integrity select new.id, e.value from json_each(new.elems) as e;
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
                id int not null references acl_id(id) on delete restrict on update restrict,
                op text references operation(name) on delete restrict on update restrict,
                integrity int references integrity_id(id) on delete restrict on update restrict,
                unique (id, op, integrity)
            ) strict)",
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
                id int not null references int_fun_id(id) on delete restrict on update restrict,
                cmp int not null references integrity_id(id) on delete restrict on update restrict,
                plus int references integrity_id(id) on delete restrict on update restrict,
                unique (id, cmp)
            ) strict)",
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

//! Executes SOFI operation in a database
/*! \param[in] file the database file name
 * \return program exit code */
int cmd_run(std::string_view file)
{
    sqlite::connection db{std::string{file}, false};
    // Check foreign key constrains, must be set for every connection outside of transactions
    sqlite::query(db, R"(pragma foreign_keys=1)").start().next_row();
    // Read all operation requests
    std::deque<op_record> ops;
    for (auto q = std::move(sqlite::query{db, R"(select * from request order by id)"}.start());
         q.next_row() == sqlite::query::status::row;)
    {
        if (q.column_count() != 6)
            throw std::runtime_error("Unexpected number of columns in table REQUEST");
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
    // Execute operations
    for (auto& o: ops) {
        std::cout << "BEGIN " << o.id << ": " << o.comment << std::endl;
        sqlite::transaction tr{db};
        sqlite::query{db, R"(delete from request where id = ?1)"}.start().bind(1, o.id).next_row();
        // TODO
        sqlite::query{db, R"(insert into result values (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10))"}.start().
            bind(1, o.id).bind(2, o.subject).bind(3, o.object).bind(4, o.op->name()).bind(5, o.arg).bind(6, o.comment).
            bind(7, o.allowed).bind(8, o.access).bind(9, o.min).bind(10, o.error).
            next_row();
        tr.commit();
        std::cout << "END   " << o.id << " allowed=" << o.allowed <<
            " access=" << o.access << " min=" << o.min << " error=" << o.error << std::endl;
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
