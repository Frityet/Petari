// Preserve original Game execution bytes without editing its source files.
// Clang performs preprocessing; this tool only records final literal provenance
// and token offsets in Clang's ordinary preprocessed output.
#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/Utils.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/MacroInfo.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <filesystem>
#include <unordered_map>
#include <string>
#include <vector>

namespace {
struct Literal {
    std::string spelling;
    std::string kind;
    std::string file;
    unsigned line;
    bool game;
};
struct Output {
    std::string preprocessed;
    std::string metadata;
    std::vector<std::string> roots;
    std::vector<std::string> files;
    bool okay = false;
};

bool is_literal(const clang::Token& token) {
    return clang::tok::isStringLiteral(token.getKind()) ||
           token.isOneOf(clang::tok::char_constant, clang::tok::wide_char_constant,
                         clang::tok::utf8_char_constant, clang::tok::utf16_char_constant,
                         clang::tok::utf32_char_constant);
}

std::string canonical(const std::string& path) {
    if (path.empty() || path.front() == '<') return path;
    return std::filesystem::weakly_canonical(path).string();
}

// Preprocessor expressions execute before the emitted C++ stream exists.
// Reject their unsupported non-ASCII ordinary character constants explicitly;
// do not silently choose a branch using UTF-8 bytes instead of original bytes.
class ConditionalGuard final : public clang::PPCallbacks {
public:
    ConditionalGuard(clang::Preprocessor& pp, const Output& output) : pp_(pp), output_(output) {}

    void MacroDefined(const clang::Token& name, const clang::MacroDirective* directive) override {
        if (!admitted(name.getLocation())) return;
        for (const auto& token : directive->getMacroInfo()->tokens())
            check_character(token, name.getLocation());
    }
    void If(clang::SourceLocation location, clang::SourceRange range, ConditionValueKind) override {
        check_condition(location, range);
    }
    void Elif(clang::SourceLocation location, clang::SourceRange range, ConditionValueKind,
              clang::SourceLocation) override {
        check_condition(location, range);
    }

private:
    bool admitted(clang::SourceLocation location) {
        auto& sources = pp_.getSourceManager();
        const auto file = canonical(sources.getFilename(sources.getSpellingLoc(location)).str());
        for (const auto& root : output_.roots)
            if (file == root || (file.starts_with(root) && file.size() > root.size() && file[root.size()] == '/')) return true;
        for (const auto& original : output_.files) if (file == original) return true;
        return false;
    }
    void report(clang::SourceLocation location) {
        const auto diagnostic = pp_.getDiagnostics().getCustomDiagID(clang::DiagnosticsEngine::Error,
            "Game execution charset cannot encode an ordinary non-ASCII character constant before preprocessor evaluation");
        pp_.Diag(location, diagnostic);
    }
    void check_character(const clang::Token& token, clang::SourceLocation diagnostic) {
        if (!token.is(clang::tok::char_constant)) return;
        const auto spelling = pp_.getSpelling(token);
        if (std::any_of(spelling.begin(), spelling.end(), [](unsigned char byte) { return byte > 127; }) ||
            spelling.find("\\u") != std::string::npos || spelling.find("\\U") != std::string::npos ||
            spelling.find("\\N") != std::string::npos) report(diagnostic);
    }
    void check_condition(clang::SourceLocation location, clang::SourceRange range) {
        if (!admitted(location)) return;
        const auto source = clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(range),
                                                      pp_.getSourceManager(), pp_.getLangOpts());
        // A raw lexer keeps identifiers and escaped character literals distinct.
        auto buffer = llvm::MemoryBuffer::getMemBufferCopy(source);
        const auto bytes = buffer->getBuffer();
        clang::Lexer lexer(clang::SourceLocation::getFromRawEncoding(1), pp_.getLangOpts(),
                           bytes.begin(), bytes.begin(), bytes.end());
        while (true) {
            clang::Token token;
            lexer.LexFromRawLexer(token);
            if (token.is(clang::tok::eof)) break;
            if (!token.is(clang::tok::char_constant)) continue;
            const auto offset = token.getLocation().getRawEncoding() - 1;
            const auto spelling = bytes.substr(offset, token.getLength());
            if (std::any_of(spelling.begin(), spelling.end(), [](unsigned char byte) { return byte > 127; }) ||
                spelling.contains("\\u") || spelling.contains("\\U") || spelling.contains("\\N")) report(location);
        }
    }
    clang::Preprocessor& pp_;
    const Output& output_;
};

class Action final : public clang::PreprocessorFrontendAction {
public:
    explicit Action(Output& output) : output_(output) {}

private:
    void ExecuteAction() override {
        auto& compiler = getCompilerInstance();
        auto& preprocessor = compiler.getPreprocessor();
        auto& sources = compiler.getSourceManager();
        preprocessor.addPPCallbacks(std::make_unique<ConditionalGuard>(preprocessor, output_));
        std::vector<Literal> literals;
        std::unordered_map<std::string, std::string> paths;
        preprocessor.setTokenWatcher([&](const clang::Token& token) {
            if (!is_literal(token)) return;
            auto location = sources.getSpellingLoc(token.getLocation());
            // # stringizing and ## token pasting use scratch storage. Their
            // execution domain is the source invoking that macro expansion.
            if (sources.isWrittenInScratchSpace(location))
                location = sources.getExpansionLoc(token.getLocation());
            const auto presumed = sources.getPresumedLoc(location);
            const auto original = sources.getFilename(location).str();
            auto [path, inserted] = paths.try_emplace(original);
            if (inserted) path->second = canonical(original);
            const auto& file = path->second;
            bool game = false;
            for (const auto& root : output_.roots)
                game |= file == root || (file.starts_with(root) && file.size() > root.size() && file[root.size()] == '/');
            for (const auto& admitted : output_.files) game |= file == admitted;
            literals.push_back({preprocessor.getSpelling(token), clang::tok::getTokenName(token.getKind()),
                                file, presumed.isValid() ? presumed.getLine() : 0U, game});
        });
        std::string text;
        llvm::raw_string_ostream stream(text);
        auto options = compiler.getPreprocessorOutputOpts();
        options.ShowCPP = true;
        options.ShowLineMarkers = true;
        options.ShowMacros = false;
        options.ShowComments = false;
        options.ShowMacroComments = false;
        clang::DoPrintPreprocessedInput(preprocessor, &stream, options);
        preprocessor.setTokenWatcher(nullptr);
        if (compiler.getDiagnostics().hasErrorOccurred()) return;

        // Tokenize the emitted stream with the same Clang lexer. Directive
        // strings (#line paths and pragma arguments) are not execution literals.
        auto buffer = llvm::MemoryBuffer::getMemBuffer(text, "<preprocessed>", false);
        const auto bytes = buffer->getBuffer();
        clang::Lexer lexer(clang::SourceLocation::getFromRawEncoding(1), compiler.getLangOpts(),
                           bytes.begin(), bytes.begin(), bytes.end());
        lexer.SetCommentRetentionState(true);
        llvm::json::Array records;
        std::size_t index = 0;
        unsigned sequence = 0;
        bool directive = false;
        while (true) {
            clang::Token token;
            lexer.LexFromRawLexer(token);
            if (token.is(clang::tok::eof)) break;
            if (token.is(clang::tok::comment)) continue;
            if (token.isAtStartOfLine()) directive = token.is(clang::tok::hash);
            if (directive) continue;
            const auto offset = token.getLocation().getRawEncoding() - 1;
            if (is_literal(token)) {
                const auto spelling = bytes.substr(offset, token.getLength());
                if (index >= literals.size() || literals[index].spelling != spelling ||
                    literals[index].kind != clang::tok::getTokenName(token.getKind())) {
                    llvm::errs() << "Game execution charset: Clang output/provenance mismatch at literal " << index
                                 << ": " << spelling << '\n';
                    if (index < literals.size()) llvm::errs() << "Expected: " << literals[index].spelling << '\n';
                    return;
                }
                const auto& literal = literals[index++];
                records.push_back(llvm::json::Object{
                    {"begin", offset}, {"end", offset + token.getLength()}, {"kind", literal.kind},
                    {"sequence", sequence}, {"game", literal.game}, {"file", literal.file}, {"line", literal.line}
                });
            }
            ++sequence;
        }
        if (index != literals.size()) {
            llvm::errs() << "Game execution charset: unmatched final literal provenance records\n";
            return;
        }
        std::error_code error;
        llvm::raw_fd_ostream pp(output_.preprocessed, error);
        if (error) { llvm::errs() << error.message() << '\n'; return; }
        pp << text;
        pp.close();
        llvm::raw_fd_ostream metadata(output_.metadata, error);
        if (error) { llvm::errs() << error.message() << '\n'; return; }
        metadata << llvm::formatv("{0}\n", llvm::json::Value(llvm::json::Object{{"tokens", std::move(records)}}));
        metadata.close();
        output_.okay = true;
    }
    Output& output_;
};
} // namespace

int main(int argc, char** argv) {
    Output output;
    std::vector<std::string> command;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--") {
            for (++i; i < argc; ++i) command.emplace_back(argv[i]);
            break;
        }
        if (i + 1 >= argc) { llvm::errs() << "Missing option argument\n"; return 2; }
        if (argument == "--output") output.preprocessed = argv[++i];
        else if (argument == "--metadata") output.metadata = argv[++i];
        else if (argument == "--game-root") output.roots.push_back(canonical(argv[++i]));
        else if (argument == "--game-file") output.files.push_back(canonical(argv[++i]));
        else { llvm::errs() << "Unknown option: " << argument << '\n'; return 2; }
    }
    if (output.preprocessed.empty() || output.metadata.empty() || (output.roots.empty() && output.files.empty()) || command.empty()) {
        llvm::errs() << "Usage: game-literal-preprocessor --output file.ii --metadata file.json --game-root path -- clang++ [arguments]\n";
        return 2;
    }
    clang::FileSystemOptions filesystem;
    llvm::IntrusiveRefCntPtr<clang::FileManager> files(new clang::FileManager(filesystem));
    clang::tooling::ToolInvocation invocation(command, std::make_unique<Action>(output), files.get());
    const bool result = invocation.run();
    return result && output.okay ? 0 : 1;
}

// Isolated tool fingerprint mutation.
