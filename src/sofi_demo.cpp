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
        // primary key or have a unique index.
        R"(create table integrity_id (id integer primary key))",
        // Stores integrity values. Rows with the same ID define a single
        // integrity, either as a non-empty set of strings, or as the empty set
        // (if there is one row with EMPTY=TRUE), or as the universe set
        // (lattice maximum, if there is one row with UNIVERSE=TRUE)
        R"(create table integrity (
                id int references integrity_id(id) on delete restrict on update restrict,
                elem text default '', empty int default false, universe int default false,
                primary key (id, elem, empty, universe),
                constraint id_not_negative check (id >= 0),
                constraint empty_bool check (empty == false or empty == true),
                constraint universe_bool check (universe == false or universe == true),
                constraint elem_or_empty_or_universe check (
                    (elem != '' and not empty and not universe) or
                    (elem == '' and empty and not universe) or
                    (elem == '' and not empty and universe)
                )
            ) without rowid, strict)",
        // Insertable JSON view of integrity values stored in table INTEGRITY,
        // one row for each integrity, represented as an (possibly empty) array
        // of strings, or a single string "universe"
        R"(create view integrity_json(id, elems) as
            select
                i.id,
                case
                    when exists (select 1 from integrity where id == i.id and universe) then json_quote('universe')
                    when exists (select 1 from integrity where id == i.id and elem != '') then
                        (select json_group_array(elem) from integrity where id == i.id)
                    else json_array()
                end
            from integrity as i group by id)",
        R"(create trigger integrity_json_insert instead of insert on integrity_json
            begin
                insert or ignore into integrity_id values (new.id);
                insert into integrity(id, universe) select new.id, 1 where new.elems == json_quote('universe');
                insert into integrity(id, empty) select new.id, 1 where new.elems == json_array();
                insert into integrity(id, elem)
                    select new.id, e.value from json_each(new.elems) as e where json_type(new.elems) == 'array';
            end)",
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
                ) stored
            ) without rowid, strict)",
        // Stores IDs of ACL values. This table is needed in order to use
        // ACL IDs as a foreign key, because a foreign key must be the
        // primary key or have a unique index.
        R"(create table acls_id (id integer primary key))",
        // Stores ACLs. Rows with the same ID define a single ACL with
        // semantics of soficpp::ops_acl containing soficpp::acl. That is,
        // there is an entry for each operation OP, and a default entry (with
        // NULL OP) controlling operations without specific entries. Each entry
        // is a (possibly empty, represented by NULL) set of integrities INT_ID. 
        R"(create table acls (
                id int not null references acls_id(id) on delete restrict on update restrict,
                op text references operations(name) on delete restrict on update restrict,
                integrity int references integrity_id(id) on delete restrict on update restrict
            ) strict)",
        // Insertable view of table ACLS that automatically adds missing ACL
        // IDs to table ACLS_ID
        R"(create view acls_ins as select * from acls)",
        R"(create trigger acls_ins_insert instead of insert on acls_ins
            begin
                insert or ignore into acls_id values (new.id);
                insert into acls values (new.id, new.op, new.integrity);
            end)",
        // Read-only view of ACLs that displays integrities in JSON format
        R"(create view acls_json(id, op, integrity) as
            select acls.id as id , acls.op as op, integrity_json.elems as integrity
            from acls join integrity_json on acls.integrity = integrity_json.id
            order by id, op)",
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
