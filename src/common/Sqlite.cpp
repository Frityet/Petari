#include "Sqlite.hpp"

#include <sqlite3.h>

#include <cstddef>
#include <stdexcept>

namespace smgpc::sql {

    Database::Database(const std::filesystem::path &path) {
        if (!path.parent_path().empty()) {
            std::filesystem::create_directories(path.parent_path());
        }

        if (sqlite3_open(path.string().c_str(), &mDb) != SQLITE_OK) {
            const auto message = mDb != nullptr ? sqlite3_errmsg(mDb) : "unknown sqlite open failure";
            throw std::runtime_error("could not open SQLite database " + path.string() + ": " + message);
        }
    }

    Database::~Database() {
        if (mDb != nullptr) {
            sqlite3_close(mDb);
        }
    }

    sqlite3 *Database::get() const {
        return mDb;
    }

    void Database::exec(std::string_view sql) {
        char *error = nullptr;
        if (sqlite3_exec(mDb, std::string(sql).c_str(), nullptr, nullptr, &error) != SQLITE_OK) {
            auto message = std::string(error != nullptr ? error : sqlite3_errmsg(mDb));
            sqlite3_free(error);
            throw std::runtime_error("SQLite exec failed: " + message);
        }
    }

    std::int64_t Database::last_insert_rowid() const {
        return sqlite3_last_insert_rowid(mDb);
    }

    Statement::Statement(Database &db, std::string_view sql) : mDb(db.get()) {
        if (sqlite3_prepare_v2(mDb, std::string(sql).c_str(), -1, &mStatement, nullptr) != SQLITE_OK) {
            throw std::runtime_error("SQLite prepare failed: " + std::string(sqlite3_errmsg(mDb)));
        }
    }

    Statement::~Statement() {
        if (mStatement != nullptr) {
            sqlite3_finalize(mStatement);
        }
    }

    void Statement::reset() {
        sqlite3_reset(mStatement);
        sqlite3_clear_bindings(mStatement);
    }

    void Statement::bind(int index, std::nullptr_t) {
        check(sqlite3_bind_null(mStatement, index));
    }

    void Statement::bind(int index, std::int64_t value) {
        check(sqlite3_bind_int64(mStatement, index, value));
    }

    void Statement::bind(int index, std::string_view value) {
        check(sqlite3_bind_text(mStatement, index, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT));
    }

    void Statement::bind_optional_int(int index, std::optional<std::int64_t> value) {
        if (value.has_value()) {
            bind(index, *value);
        } else {
            bind(index, nullptr);
        }
    }

    void Statement::bind_optional_text(int index, const std::optional<std::string> &value) {
        if (value.has_value()) {
            bind(index, std::string_view(*value));
        } else {
            bind(index, nullptr);
        }
    }

    bool Statement::step() {
        const auto result = sqlite3_step(mStatement);
        if (result == SQLITE_ROW) {
            return true;
        }
        if (result == SQLITE_DONE) {
            reset();
            return false;
        }
        throw std::runtime_error("SQLite step failed: " + std::string(sqlite3_errmsg(mDb)));
    }

    void Statement::step_done() {
        const auto result = sqlite3_step(mStatement);
        if (result != SQLITE_DONE) {
            throw std::runtime_error("SQLite step failed: " + std::string(sqlite3_errmsg(mDb)));
        }
        reset();
    }

    std::optional<std::int64_t> Statement::column_int(int index) const {
        if (sqlite3_column_type(mStatement, index) == SQLITE_NULL) {
            return std::nullopt;
        }
        return sqlite3_column_int64(mStatement, index);
    }

    std::optional<std::string> Statement::column_text(int index) const {
        if (sqlite3_column_type(mStatement, index) == SQLITE_NULL) {
            return std::nullopt;
        }

        const auto *text = sqlite3_column_text(mStatement, index);
        const auto length = sqlite3_column_bytes(mStatement, index);
        if (text == nullptr) {
            return std::string {};
        }
        return std::string(reinterpret_cast<const char *>(text), static_cast<std::size_t>(length));
    }

    void Statement::check(int result) {
        if (result != SQLITE_OK) {
            throw std::runtime_error("SQLite bind failed: " + std::string(sqlite3_errmsg(mDb)));
        }
    }

}  // namespace smgpc::sql
