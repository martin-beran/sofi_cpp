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

#include "soficpp/soficpp.hpp"
#include "sqlite_cpp.hpp"

#include <iostream>

namespace {

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
        // Stores integrity values. Rows with the same ID define a single
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
        // Stores operation definitions, identified by operation NAME.
        R"(create table operations (
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
        // Insert one operation of each kind
        R"(insert into operations values
                ('no_flow', false, false), ('read', true, false), ('write', false, true), ('read_write', true, true))",
        // Stores IDs of ACL values. This table is needed in order to use
        // ACL IDs as a foreign key, because a foreign key must be the
        // primary key or have a unique index.
        R"(create table acls_id (
                id integer primary key,
                constraint acl_id_not_negative check (id >= 0)
            ))",
        // Stores ACLs. Rows with the same ID define a single ACL with
        // semantics of soficpp::ops_acl containing soficpp::acl. That is,
        // there is an entry for each operation OP, and a default entry (with
        // NULL OP) controlling operations without specific entries. Each entry
        // is a (possibly empty, represented by NULL) set of integrities INT_ID. 
        R"(create table acls (
                id int not null references acls_id(id) on delete restrict on update restrict,
                op text references operations(name) on delete restrict on update restrict,
                integrity int references integrity_id(id) on delete restrict on update restrict,
                unique (id, op, integrity)
            ) strict)",
        // Insertable view of table ACLS that automatically adds missing ACL
        // IDs to table ACLS_ID
        R"(create view acls_ins as select * from acls)",
        R"(create trigger acls_ins_insert instead of insert on acls_ins
            begin
                insert into acls_id values (new.id) on conflict do nothing;
                insert into acls values (new.id, new.op, new.integrity);
            end)",
        // Read-only view of ACLs that displays integrities in JSON format
        R"(create view acls_json(id, op, integrity) as
            select acls.id as id , acls.op as op, integrity_json.elems as integrity
            from acls left join integrity_json on acls.integrity = integrity_json.id
            order by id, op)",
        // Insert ACLs that deny all operations and allow all operations
        R"(insert into acls_ins values (0, null, null), (1, null, 0))",
        // Read-only view of ACLs that selects values usable as minimum integrity
        R"(create view min_integrity as select id, integrity from acls where op is null)",
        // JSON value of MIN_INTEGRITY
        R"(create view min_integrity_json as select id, integrity from acls_json where op is null)",
        // Stores IDs of INT_FUN values. This table is needed in order to use
        // integrity function IDs as a foreign key, because a foreign key must
        // be the primary key or have a unique index.
        R"(create table int_fun_id (
                id integer primary key,
                comment text default '',
                constraint int_fun_id_not_negative check (id >= 0)
            ))",
        // Stores integrity modification functions, usable as test, providing,
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
    }) {
        sqlite::query(db, sql).start().next_row();
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
