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
    sqlite::transaction tr{db};
    for (const auto& sql: {
        // Integrity reserved ids: 0 = empty set (minimum), -1 = universe (maximum)
        R"(create table integrity (id int, elem text, primary key (id, elem), check (id > 0)) without rowid, strict)",
        R"(create view integrity_json(id, elems) as
            select 0, json_array() union
            select -1, "universe" union
            select id, json_group_array(elem) from integrity group by id)",
        R"(create trigger integrity_json_insert instead of insert on integrity_json
            begin
                insert into integrity select new.id, e.value from json_each(new.elems) as e;
            end)",
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
