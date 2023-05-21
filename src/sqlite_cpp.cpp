/*! \file
 * \brief Implementation part of sqlite_cpp.hpp
 */

#include "sqlite_cpp.hpp"

#include <cassert>
#include <iostream>
#include <sqlite3.h>
#include <utility>

namespace sqlite {

// In sqlite_cpp.hpp, query is declared before connection, because a connection
// contains queries for starting and finishing a transaction. The order of
// classes is different here, because query::impl needs connection and
// connection::impl to be already complete types.

/*** connection::impl ********************************************************/

//! Internal implementation class for sqlite::connection
class connection::impl {
public:
    //! Creates a new database connection.
    /*! \param[in] conn the related interface object
     * \param[in] create whether to create the database if it does not exist */
    explicit impl(connection& conn, bool create);
    //! No copy
    impl(const impl&) = delete;
    //! No move
    impl(impl&&) = delete;
    //! Closes the database connection.
    ~impl();
    //! No copy
    impl& operator=(const impl&) = delete;
    //! No move
    impl& operator=(impl&&) = delete;
    //! The sqlite::connection object that owns this object
    connection& conn;
    //! The native SQLite connection handle
    sqlite3* db = nullptr;
};

connection::impl::impl(connection& conn, bool create): conn(conn)
{
    if (int status = sqlite3_open_v2(conn._file.c_str(), &db,
                                     (create ? SQLITE_OPEN_CREATE : 0U) |
                                     SQLITE_OPEN_READWRITE |
                                     SQLITE_OPEN_URI |
                                     SQLITE_OPEN_NOMUTEX |
                                     SQLITE_OPEN_PRIVATECACHE |
                                     SQLITE_OPEN_EXRESCODE,
                                     nullptr);
        status != SQLITE_OK)
    {
        if (!db)
            throw error("sqlite3_open_v2", conn._file);
        throw error("sqlite3_open_v2", conn, *this);
    }
    if (int status = sqlite3_exec(db, "pragma synchronous = normal", nullptr,
                                  nullptr, nullptr);
        status != SQLITE_OK)
    {
        throw error("sqlite3_exec(pragma)", conn, *this);
    }
    assert(db);
}

connection::impl::~impl()
{
    if (sqlite3_close_v2(db) != SQLITE_OK) {
        assert(db);
        std::cerr << "Cannot close database \"" << conn._file << "\" handle: " << sqlite3_errmsg(db);
    }
}

/*** connection **************************************************************/

connection::connection(std::string file, bool create):
    _file(std::move(file)), _impl(std::make_unique<impl>(*this, create)),
    _transaction_begin_deferred(*this, "begin deferred transaction"),
    _transaction_begin_immediate(*this, "begin immediate transaction"),
    _transaction_begin_exclusive(*this, "begin exclusive transaction"),
    _transaction_commit(*this, "commit transaction"),
    _transaction_rollback(*this, "rollback transaction")
{
}

connection::~connection() = default;

void connection::interrupt()
{
    if (_impl->db)
        sqlite3_interrupt(_impl->db);
}

/*** query::impl *************************************************************/

//! Internal implementation class for sqlite::query
class query::impl {
public:
    //! Creates a prepared SQL query
    /*! \param[in] q the related interface object */
    explicit impl(query& q);
    //! No copy
    impl(const impl&) = delete;
    //! No move
    impl(impl&&) = delete;
    //! Destroys the prepared statement and frees associated resources
    ~impl();
    //! No copy
    impl& operator=(const impl&) = delete;
    //! No move
    impl& operator=(impl&&) = delete;
    //! The sqlite::query object that owns this object
    [[maybe_unused]] query& q;
    //! The native SQLite prepared statement handle
    sqlite3_stmt* stmt = nullptr;
};

query::impl::impl(query& q): q(q)
{
    // sqlite3 allows passing size incl. terminating NUL
    if (sqlite3_prepare_v3(q._db._impl->db, q._sql.c_str(),
                           int(q._sql.size() + 1),
                           SQLITE_PREPARE_PERSISTENT, &stmt,
                           nullptr) != SQLITE_OK)
    {
        assert(!stmt);
        throw error("sqlite3_prepare_v3", q._db, q._sql);
    }
    assert(stmt);
}

query::impl::~impl()
{
    sqlite3_finalize(stmt);
}

/*** query *******************************************************************/

query::query(connection& db, std::string sql):
    _db(db), _sql(std::move(sql)), _impl(std::make_unique<impl>(*this))
{
}

query::query(query&&) noexcept = default;

query::~query() = default;

query& query::bind(int i, std::nullptr_t)
{
    if (sqlite3_bind_null(_impl->stmt, i) != SQLITE_OK)
        throw error("sqlite3_bind_null", _db, _sql);
    return *this;
}

query& query::bind(int i, bool v)
{
    if (sqlite3_bind_int64(_impl->stmt, i, int64_t(v)) != SQLITE_OK)
        throw error("sqlite3_bind_int64", _db, _sql);
    return *this;
}

query& query::bind(int i, int64_t v)
{
    if (sqlite3_bind_int64(_impl->stmt, i, v) != SQLITE_OK)
        throw error("sqlite3_bind_int64", _db, _sql);
    return *this;
}

query& query::bind(int i, double v)
{
    if (sqlite3_bind_double(_impl->stmt, i, v) != SQLITE_OK)
        throw error("sqlite3_bind_double", _db, _sql);
    return *this;
}

query& query::bind(int i, const std::string& v)
{
    if (sqlite3_bind_text(_impl->stmt, i, v.c_str(), int(v.size()), SQLITE_STATIC) != SQLITE_OK)
        throw error("sqlite3_bind_text", _db, _sql);
    return *this;
}

query& query::bind(int i, std::string_view v)
{
    if (sqlite3_bind_text(_impl->stmt, i, v.data(), int(v.size()), SQLITE_STATIC) != SQLITE_OK)
        throw error("sqlite3_bind_text", _db, _sql);
    return *this;
}

query& query::bind(int i, const blob_t& v)
{
    if (sqlite3_bind_blob(_impl->stmt, i, v.data(), int(v.size()), SQLITE_STATIC) != SQLITE_OK)
        throw error("sqlite3_bind_blob", _db, _sql);
    return *this;
}

int query::column_count()
{
    return sqlite3_column_count(_impl->stmt);
}

query::column_value query::get_column(int i)
{
    switch (sqlite3_column_type(_impl->stmt, i)) {
    case SQLITE_NULL:
    default:
        return nullptr;
    case SQLITE_INTEGER:
        return sqlite3_column_int64(_impl->stmt, i);
    case SQLITE_FLOAT:
        return sqlite3_column_double(_impl->stmt, i);
    case SQLITE_TEXT:
        return std::string(reinterpret_cast<const char*>(sqlite3_column_text(_impl->stmt, i)),
                           size_t(sqlite3_column_bytes(_impl->stmt, i)));
    case SQLITE_BLOB:
        {
            auto b = reinterpret_cast<const unsigned char*>(sqlite3_column_blob(_impl->stmt, i));
            return blob_t(b, b + sqlite3_column_bytes(_impl->stmt, i));
        }
    }
}

query::status query::next_row(uint32_t retries)
{
    switch (auto status = sqlite3_step(_impl->stmt); status % 256) {
    case SQLITE_DONE:
        return status::done;
    case SQLITE_ROW:
        return status::row;
    case SQLITE_BUSY:
    case SQLITE_LOCKED:
        if (retries > 0)
            return status::locked;
        [[fallthrough]];
    default:
        throw error("sqlite3_step", _db, _sql);
    }
}

query& query::start(bool restart)
{
    sqlite3_reset(_impl->stmt);
    if (!restart && sqlite3_clear_bindings(_impl->stmt) != SQLITE_OK)
        throw error("sqlite3_clear_bindings", _db, _sql);
    return *this;
}

/*** transaction *************************************************************/

transaction::transaction(connection& db, mode m): _db(db), _mode(m)
{
    query& begin = [this]() -> query& {
        switch (_mode) {
        case mode::deferred:
            return _db._transaction_begin_deferred;
        case mode::immediate:
            return _db._transaction_begin_immediate;
        case mode::exclusive:
            return _db._transaction_begin_exclusive;
        default:
            assert(false);
        }
    }();
    auto r = begin.start().next_row();
    assert(r == query::status::done);
}

transaction::~transaction()
{
    if (!_finished)
        try {
            rollback();
        } catch (const error&) {
            // ignore an SQLite error from rollback
        }
}

void transaction::commit()
{
    if (_finished)
        return;
    _finished = true;
    auto r = _db._transaction_commit.start().next_row();
    assert(r == query::status::done);
}

void transaction::rollback()
{
    if (_finished)
        return;
    _finished = true;
    auto r = _db._transaction_rollback.start().next_row();
    assert(r == query::status::done);
}

/*** error *******************************************************************/

error::error(const std::string& fun, const std::string& file):
    runtime_error("sqlite3 error in db \"" + file + "\" function " + fun + "(): Cannot allocate database handle")
{
}

error::error(const std::string& fun, connection& db, const std::string& sql):
        runtime_error("sqlite3 error in db \"" + db._file + "\"" +
                      " function " + fun + "(): " + sqlite3_errmsg(db._impl->db) +
                      "\nquery:\n" + sql)
{
}

error::error(const std::string& fun, connection& db, connection::impl& impl):
        runtime_error("sqlite3 error in db \"" + db._file + "\"" +
                      " function " + fun + "(): " + sqlite3_errmsg(impl.db))
{
}

} // namespace sqlite
