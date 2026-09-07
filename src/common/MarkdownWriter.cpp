#include "MarkdownWriter.hpp"

#include <algorithm>
#include <ostream>

namespace smgpc::dump {

    MarkdownWriter::MarkdownWriter(std::ostream &out) : _out(out) {
    }

    void MarkdownWriter::heading(unsigned level, std::string_view text) {
        level = std::clamp(level, 1U, 6U);
        for (auto i = 0U; i < level; ++i) {
            _out << '#';
        }
        _out << ' ' << text << "\n\n";
    }

    void MarkdownWriter::paragraph(std::string_view text) {
        _out << text << "\n\n";
    }

    void MarkdownWriter::blank_line() {
        _out << '\n';
    }

    std::string markdown_table_cell(std::string_view value) {
        auto escaped = std::string {};
        escaped.reserve(value.size());
        for (const auto ch : value) {
            switch (ch) {
            case '|':
                escaped += "\\|";
                break;
            case '\n':
            case '\r':
                escaped += "<br>";
                break;
            default:
                escaped += ch;
                break;
            }
        }
        return escaped;
    }

    void MarkdownWriter::table(std::span<const std::string_view> headers, std::span<const std::vector<std::string>> rows) {
        _out << '|';
        for (const auto header : headers) {
            _out << ' ' << markdown_table_cell(header) << " |";
        }
        _out << "\n|";
        for (auto i = std::size_t {}; i < headers.size(); ++i) {
            _out << " --- |";
        }
        _out << '\n';

        for (const auto &row : rows) {
            _out << '|';
            for (auto column = std::size_t {}; column < headers.size(); ++column) {
                const auto cell = column < row.size() ? std::string_view(row[column]) : std::string_view {};
                _out << ' ' << markdown_table_cell(cell) << " |";
            }
            _out << '\n';
        }
        _out << '\n';
    }

}  // namespace smgpc::dump
