// Compiler-only source inspection. Clang owns C++ token boundaries, including
// disabled preprocessor branches, comments, raw literals and line splices.
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/TokenKinds.h>
#include <clang/Lex/Lexer.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <string>

int main(int argc, char** argv) {
    clang::LangOptions options;
    options.CPlusPlus = options.CPlusPlus11 = options.CPlusPlus14 = true;
    options.CPlusPlus17 = options.CPlusPlus20 = options.CPlusPlus23 = true;
    options.LineComment = options.Bool = true;
    llvm::json::Array files;
    for (int file = 1; file < argc; ++file) {
        auto buffer = llvm::MemoryBuffer::getFile(argv[file]);
        if (!buffer) { llvm::errs() << argv[file] << ": " << buffer.getError().message() << '\n'; return 1; }
        const auto bytes = (*buffer)->getBuffer();
        clang::Lexer lexer(clang::SourceLocation::getFromRawEncoding(1), options,
                           bytes.begin(), bytes.begin(), bytes.end());
        lexer.SetCommentRetentionState(true);
        llvm::json::Array tokens;
        unsigned sequence = 0;
        bool after_hash = false;
        std::string directive;
        while (true) {
            clang::Token token;
            lexer.LexFromRawLexer(token);
            if (token.is(clang::tok::eof)) break;
            const auto offset = token.getLocation().getRawEncoding() - 1;
            if (token.is(clang::tok::comment)) continue;
            if (token.isAtStartOfLine()) { directive.clear(); after_hash = false; }
            if (token.is(clang::tok::hash) && token.isAtStartOfLine()) after_hash = true;
            else if (after_hash) {
                if (token.is(clang::tok::raw_identifier)) directive = bytes.substr(offset, token.getLength()).str();
                after_hash = false;
            }
            const bool string = clang::tok::isStringLiteral(token.getKind());
            const bool character = token.isOneOf(clang::tok::char_constant, clang::tok::wide_char_constant,
                clang::tok::utf8_char_constant, clang::tok::utf16_char_constant, clang::tok::utf32_char_constant);
            if (string || character) {
                tokens.push_back(llvm::json::Object{
                    {"begin",offset},{"end",offset + token.getLength()},
                    {"kind",clang::tok::getTokenName(token.getKind())},
                    {"sequence",sequence},{"directive",directive}
                });
            }
            ++sequence;
        }
        files.push_back(llvm::json::Object{{"path",argv[file]},{"tokens",std::move(tokens)}});
    }
    llvm::outs() << llvm::formatv("{0}\n",llvm::json::Value(std::move(files)));
}
