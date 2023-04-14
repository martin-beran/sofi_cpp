#pragma once

/*! \file
 * \brief Interface to database SQLite 3
 */

#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <variant>

//! Interface to database SQLite 3
namespace sqlite {

class connection;
class query;
class transaction;

//! A connection to a SQLite database
class connection {
public:
    // not using string_view, because sqlite3 requires null-terminated strings
    explicit connection(std::string file);
    connection(const connection&) = delete;
    connection(connection&&) = delete;
    ~connection();
    connection& operator=(const connection&) = delete;
    connection& operator=(connection&&) = delete;
    // May be called from any thread
    void interrupt();
private:
    class impl;
    std::string _file;
    std::unique_ptr<impl> _impl;
    friend class error;
    friend class query;
};

//! A prepared SQLite query
class query {
public:
    enum class column_type: int {
        ct_null = 0,
        ct_int64 = 1,
        ct_double = 2,
        ct_string = 3,
        ct_blob = 4,
    };
    enum class status {
        row,
        done,
        locked,
    };
    using column_value = std::variant<
        std::nullptr_t,
        int64_t,
        double,
        std::string,
        std::string
    >;
    explicit query(connection& db, std::string sql, std::string sql_id = {});
    query(const query&) = delete;
    query(query&&) = delete;
    ~query();
    query& operator=(const query&) = delete;
    query& operator=(query&&) = delete;
    void start(bool restart);
    void bind(int i, std::nullptr_t v);
    void bind(int i, int64_t v);
    void bind(int i, double v);
    void bind(int i, const std::string& v);
    void bind_blob(int i, const std::string& v);
    int column_count();
    status next_row(uint32_t retries);
    column_value get_column(int i);
private:
    class impl;
    connection& _db;
    std::string _sql;
    std::string _sql_id;
    std::unique_ptr<impl> _impl;
};

//! A database transaction
class transaction {
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
