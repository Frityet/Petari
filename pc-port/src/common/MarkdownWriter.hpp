#pragma once

#include <iosfwd>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace smgpc::dump {

    class MarkdownWriter final {
    public:
        explicit MarkdownWriter(std::ostream &out);

        void heading(unsigned level, std::string_view text);
        void paragraph(std::string_view text);
        void blank_line();
        void table(std::span<const std::string_view> headers, std::span<const std::vector<std::string>> rows);

    private:
        std::ostream &_out;
    };

    [[nodiscard]] std::string markdown_table_cell(std::string_view value);

}  // namespace smgpc::dump
