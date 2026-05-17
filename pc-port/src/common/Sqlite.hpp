#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

struct sqlite3;
struct sqlite3_stmt;

namespace smgpc::sql {

    class Database {
    public:
        explicit Database(const std::filesystem::path &path);
        Database(const Database &) = delete;
        Database &operator=(const Database &) = delete;
        Database(Database &&) = delete;
        Database &operator=(Database &&) = delete;
        ~Database();

        [[nodiscard]] sqlite3 *get() const;
        void exec(std::string_view sql);
        [[nodiscard]] std::int64_t last_insert_rowid() const;

    private:
        sqlite3 *mDb = nullptr;
    };

    class Statement {
    public:
        Statement(Database &db, std::string_view sql);
        Statement(const Statement &) = delete;
        Statement &operator=(const Statement &) = delete;
        Statement(Statement &&) = delete;
        Statement &operator=(Statement &&) = delete;
        ~Statement();

        void reset();
        void bind(int index, std::nullptr_t);
        void bind(int index, std::int64_t value);
        void bind(int index, std::string_view value);
        void bind_optional_int(int index, std::optional<std::int64_t> value);
        void bind_optional_text(int index, const std::optional<std::string> &value);
        void step_done();

    private:
        void check(int result);

        sqlite3 *mDb = nullptr;
        sqlite3_stmt *mStatement = nullptr;
    };

}  // namespace smgpc::sql
