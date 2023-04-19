#pragma once

/*! \file
 * \brief Interface to database SQLite 3
 */

#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <variant>
#include <vector>

//! Interface to database SQLite 3
/*! This is only a thin wrapper around the native SQLite C API. Consult
 * documentation at https://sqlite.org/c3ref/intro.html for more information
 * about SQLite API.
 *
 * Objects may be destroyed in any order. All
 * transactions (sqlite::trasaction) and prepared statements (sqlite::query)
 * must be destroyed before closing the related database connection
 * (by the destructor of sqlite::connection).
 *
 * Member functions of classes in this namespace and in nested namespace
 * sqlite::impl report errors by throwing exceptions:
 * \throw sqlite::error if an SQLite API call fails
 * \throw std::bad_alloc if an allocation fails, for example, an allocation of
 * an internal implementation object */
namespace sqlite {

class connection;
class query;
class transaction;

//! The type used for blob values
/*! \note It must a different type than \c std::string. */
using blob_t = std::vector<unsigned char>;

//! A prepared SQLite query
/*! The query object can be executed multiple times. Before each execution,
 * function start() must be called. Then, if the query has any parameters, they
 * are assigned values by repeatedly calling a bind() or bind_blob() function.
 * The query is executed and rows of the query result are obtained by
 * repeatedly calling next_row(). Column values are extracted from a row by
 * get_column(), with column_count() returning the number of columns. */
class query {
public:
    //! Supported data types of database values and columns
    enum class column_type: int {
        ct_null = 0, //!< A \c NULL value
        ct_int64 = 1, //!< Signed 64bit integer
        ct_double = 2, //!< Floating point 8byte IEEE value
        ct_string = 3, //!< A text string in the database encoding (usually UTF-8)
        ct_blob = 4, //!< A binary blob
    };
    //! Possible result of query execution
    enum class status {
        row, //!< The next row of the result is available
        done, //!< Query processing has been finished
        locked, //!< The database is locked
    };
    //! A single value that can be stored in a database or returned by a query
    /*! It is a variant containing types depending on value type:
     * \arg column_type::ct_null -- \c std::nullptr
     * \arg column_type::ct_int64 -- \c int64_t
     * \arg column_type::ct_double -- \c double
     * \arg column_type::ct_string -- \c std::string
     * \arg column_type::ct_blob -- \c blob_t */
    using column_value = std::variant<
        std::nullptr_t,
        int64_t,
        double,
        std::string,
        blob_t
    >;
    //! Creates a prepared SQL query
    /*! It just prepares the query, but does not start its execution.
     * \param[in] db a database connection
     * \param[in] sql the query text in SQL */
    explicit query(connection& db, std::string sql);
    //! No copy
    query(const query&) = delete;
    //! Default move
    query(query&&) noexcept;
    //! Destroys the prepared statement and frees associated resources
    ~query();
    //! No copy
    query& operator=(const query&) = delete;
    //! No move
    query& operator=(query&&) = delete;
    //! Starts execution of the query.
    /*! For each execution of the query, it should be called once with \a
     * restart equal to \c false, which clears any existing parameter bindings.
     * If called to restart the query after next_row() returned status::locked,
     * \a restart should be \c true, which keeps any existing parameter
     * bindings.
     * \param[in] restart indicates whether this is the initial call (\c false)
     * or a restart after status::locked
     * \return \c *this */
    query& start(bool restart = false);
    //! Binds a query parameter to the \c NULL value
    /*! \param[in] i parameter index (starting from 1)
     * \param[in] v the parameter value (always \c std::nullptr)
     * \return \c *this */
    query& bind(int i, std::nullptr_t v);
    //! Binds a query parameter to a Boolean value converted to integer
    /*! \param[in] i parameter index (starting from 1)
     * \param[in] v the parameter value
     * \return \c *this */
    query& bind(int i, bool v);
    //! Binds a query parameter to an integer value
    /*! \param[in] i parameter index (starting from 1)
     * \param[in] v the parameter value
     * \return \c *this */
    query& bind(int i, int64_t v);
    //! Binds a query parameter to a floating point value
    /*! \param[in] i parameter index (starting from 1)
     * \param[in] v the parameter value
     * \return \c *this */
    query& bind(int i, double v);
    //! Binds a query parameter to a string value
    /*! \param[in] i parameter index (starting from 1)
     * \param[in] v the parameter value
     * \return \c *this */
    query& bind(int i, const std::string& v);
    //! Binds a query parameter to a string view value
    /*! \param[in] i parameter index (starting from 1)
     * \param[in] v the parameter value
     * \return \c *this */
    query& bind(int i, std::string_view v);
    //! Binds a query parameter to a blob value
    /*! \param[in] i parameter index (starting from 1)
     * \param[in] v the parameter value
     * \return \c *this */
    query& bind(int i, const blob_t& v);
    //! Gets the number of columns in the query result.
    /*! \return the number of columns */
    int column_count();
    //! Continues the query execution until the next row is produced.
    /*! The functions should be called repeatedly while it returns status::row,
     * in order to get all rows of the result. It signals that there are no
     * more rows by returning status::done. If the database is locked, that is,
     * the low-level API function returns \c SQLITE_ROW or \c SQLITE_LOCKED,
     * the behavior is controlled by parameter \a retries. If \a retries is 0,
     * then sqlite::error is thrown. Otherwise, status::locked is returned and
     * the caller should decrement \a retries, restart the query by \c
     * start(true), and begin repeating calls to next_row() again.
     * \param[in] retries the number of planned retries after detecting a
     * locked database
     * \return intermediate (status::row) or final (status::done,
     * status::locked) result of query execution */
    status next_row(uint32_t retries = 0);
    //! Gets a column value from a row returned by the query
    /*! \param[in] i column index (starting from 0); must be between 0 and
     * <tt>column_count()-1</tt>, inclusive
     * \return the column value */
    column_value get_column(int i);
private:
    class impl;
    connection& _db; //!< The database connection owning this prepared statement
    std::string _sql; //!< The original SQL text of the statement
    std::unique_ptr<impl> _impl; //!< Internal implementation object (PIMPL)
};

//! A connection to a SQLite database
class connection {
public:
    //! Creates a new database connection.
    /*! \param[in] file a database file name
     * \param[in] create whether to create the database if it does not exist
     * \throw error if the database does not exist and \a create is \c false
     * \note We are not using \c std::string_view, because sqlite3 requires
     * null-terminated strings. */
    explicit connection(std::string file, bool create);
    //! No copy
    connection(const connection&) = delete;
    //! No move
    connection(connection&&) = delete;
    //! Closes the database connection.
    ~connection();
    //! No copy
    connection& operator=(const connection&) = delete;
    //! No move
    connection& operator=(connection&&) = delete;
    //! Aborts any pending database operation.
    /*! \threadsafe{safe, safe} */
    void interrupt();
private:
    class impl;
    std::string _file; //!< Database file name
    std::unique_ptr<impl> _impl; //!< Internal implementation object (PIMPL)
    query _transaction_begin_deferred; //!< Used by \ref transaction
    query _transaction_begin_immediate; //!< Used by \ref transaction
    query _transaction_begin_exclusive; //!< Used by \ref transaction
    query _transaction_commit; //!< Used by \ref transaction
    query _transaction_rollback; //!< Used by \ref transaction
    friend class error;
    friend class query;
    friend class transaction;
};

//! A database transaction
/*! \todo Review/fix transaction handling in the case of retried queries after
 * detecting a locked database. Generally, a transaction should be rolled back
 * after a statement in the transaction fails. */
class transaction {
public:
    //! The transaction mode
    enum class mode {
        deferred, //!< Starts the transaction by <tt>BEGIN DEFERRED TRANSACTION</tt>
        immediate, //!< Starts the transaction by <tt>BEGIN IMMEDIATE TRANSACTION</tt>
        exclusive, //!< Starts the transaction by <tt>BEGIN EXCLUSIVE TRANSACTION</tt>
    };
    //! Starts a new transaction.
    /*! \param[in] db a database connection
     * \param[in] m the transaction mode */
    explicit transaction(connection& db, mode m = mode::deferred);
    //! Terminates the transaction.
    /*! If commit() or rollback() has been called, it does nothing. Otherwise,
     * it rolls back the transaction. */
    ~transaction();
    //! Commits the transaction.
    /*! It executes <tt>COMMIT TRANSACTION</tt>. It does nothing if commit() or
     * rollback() has been already called. */
    void commit();
    //! Rolls back the transaction.
    /*! It executes <tt>ROLLBACK TRANSACTION</tt>. It does nothing if commit()
     * or rollback() has been already called. */
    void rollback();
private:
    connection& _db; //!< The database connection owning this transaction
    mode _mode; //!< The transaction mode
    bool _finished = false; //!< If commit() or rollback() has been called
};

//! An exception thrown if an SQLite operation returns an error
class error: public std::runtime_error {
public:
    //! Stores information about the error.
    /*! It can be used if the internal \c sqlite3 structure is not allocated.
     * \param[in] fun the name of the failed SQLite function
     * \param[in] file the database file name */
    explicit error(const std::string& fun, const std::string& file);
    //! Stores information about the error.
    /*! This is the most commonly used costructor.
     * \param[in] fun the name of the failed SQLite function
     * \param[in] db the database connection where the error occurred
     * \param[in] sql the failed SQL query */
    explicit error(const std::string& fun, connection& db, const std::string& sql = {});
    //! Stores information about the error.
    /*! It can be used before \c db._impl is initialized.
     * \param[in] fun the name of the failed SQLite function
     * \param[in] db the database connection where the error occurred
     * \param[in] impl the internal SQLite connection object */
    error(const std::string& fun, connection& db, connection::impl& impl);
};

} // namespace sqlite
