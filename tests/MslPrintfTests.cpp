#include "compat/MslPrintfCompat.hpp"
#include <array>
#include <atomic>
#include <climits>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

extern "C" int test_original_printf_alias(char *, std::size_t);
extern "C" int test_original_sprintf_alias(char *);
extern "C" int test_original_vsnprintf_alias(char *, std::size_t, const char *, ...);
extern "C" int test_original_vsprintf_alias(char *, const char *, ...);
namespace {
    void require(bool value, const char *message) {
        if (!value)
            throw std::runtime_error(message);
    }
    void equal(const char *expected, const char *format, ...) {
        char out[2048];
        va_list args;
        va_start(args, format);
        int length = smgpc_msl_vsnprintf(out, sizeof(out), format, args);
        va_end(args);
        require(length == int(std::strlen(expected)) && std::strcmp(out, expected) == 0, "independent formatted-string expectation");
    }
    void native(const char *format, ...) {
        char actual[4096], expected[4096];
        va_list args, copy;
        va_start(args, format);
        va_copy(copy, args);
        int wanted = std::vsnprintf(expected, sizeof(expected), format, args);
        int got = smgpc_msl_vsnprintf(actual, sizeof(actual), format, copy);
        va_end(copy);
        va_end(args);
        require(wanted == got && std::memcmp(actual, expected, std::size_t(wanted) + 1) == 0, "defined native numeric/extension behavior preserved");
    }
    void null_strings() {
        equal("/directory", "%s/%s", static_cast<const char *>(nullptr), "directory");
        equal("a::b", "%s:%s:%s", "a", static_cast<const char *>(nullptr), "b");
        equal("     |", "%5s|", static_cast<const char *>(nullptr));
        equal("00000|", "%05s|", static_cast<const char *>(nullptr));
        equal("|", "%.0s|", static_cast<const char *>(nullptr));
    }
    void precision_padding() {
        equal("abc   /", "%*.*s/%s", -6, 3, "abcdef", static_cast<const char *>(nullptr));
        equal("000ab", "%05.2s", "abc");
        equal("-000x", "%05s", "-x");
        equal("%512s/%d", "%512s/%d", "ignored", 17);
        equal("text", "%.*s", -1, "text");
    }
    void pascal_wide() {
        const char value[] = {5, 'a', 'b', 'c', 'd', 'e'};
        equal("  abc", "%#5.3s", value);
        equal("", "%#s", static_cast<const char *>(nullptr));
        equal("|Wide|", "%ls|%ls|", static_cast<const wchar_t *>(nullptr), L"Wide");
        equal("   Wi", "%5.2ls", L"Wide");
        char binary[] = {3, 'a', '\0', 'b'}, out[16];
        int length = smgpc_msl_snprintf(out, sizeof(out), "%#s", binary);
        require(length == 3 && out[0] == 'a' && out[1] == 0 && out[2] == 'b' && out[3] == 0, "Pascal payload length and embedded NUL preserved");
    }
    void wide_bytes() {
        const wchar_t value[] = {wchar_t(0x00e9), wchar_t(0x0141), 0};
        char out[16];
        require(smgpc_msl_snprintf(out, sizeof(out), "%ls", value) == 2 && static_cast<unsigned char>(out[0]) == 0xe9 && out[1] == 'A' && out[2] == 0, "MSL C locale copies each wide character low byte");
        const wchar_t nul[] = {wchar_t(0x0100), L'x', 0};
        require(smgpc_msl_snprintf(out, sizeof(out), "%ls", nul) == 0 && out[0] == 0, "MSL string length sees a converted embedded zero");
        const wchar_t pascal[] = {3, L'a', wchar_t(0x0100), L'b', 0};
        require(smgpc_msl_snprintf(out, sizeof(out), "%#ls", pascal) == 3 && out[0] == 'a' && out[1] == 0 && out[2] == 'b' && out[3] == 0, "MSL wide Pascal payload preserves byte count and embedded NUL");
    }
    void numeric() {
        int object = 42;
        native("%s%d %i %u %#o %#x %#X %%", "", -42, 123, 0xffffffffu, 0755u, 0xabcu, 0xabcu);
        native("%s%hhd %hd %ld %lld %zu %td %jd", "", -3, -7, -123456789L, -1234567890123LL, std::size_t(999), std::ptrdiff_t(-99), std::intmax_t(-777));
        native("%s%hu %hhu %lu %llu %ju %tu", "", 65530u, 250u, ULONG_MAX, ULLONG_MAX, std::uintmax_t(111), std::make_unsigned_t<std::ptrdiff_t>(99));
        native("%s%+.6f %.5e %.8g %a %.3Lf %p %.2lc", "", 1.25, -1e-10, 12345.6789, .125, 1.25L, &object, std::wint_t('Q'));
        native("%s%*.*f:%08x:%*d", "", 18, 7, 3.14159, 0x123u, -12, 17);
        native("%s%1000d", "", 3);
        native("%2$s:%1$d", 7, "position");
    }
    void mixed() {
        char actual[512], expected[512];
        const char *format = "%s|%*.*f|%lld|%p|%zu|%s|%.2Lf|%ls";
        int object = 0;
        auto n = smgpc_msl_snprintf(actual, sizeof(actual), format, static_cast<const char *>(nullptr), 12, 4, 1.25, -1234567890123LL, &object, std::size_t(87), "ok", 2.5L, L"wide");
        auto e = std::snprintf(expected, sizeof(expected), format, "", 12, 4, 1.25, -1234567890123LL, &object, std::size_t(87), "ok", 2.5L, L"wide");
        require(n == e && std::strcmp(actual, expected) == 0, "native variadic integer/FP/pointer banks remain in order after null string");
    }
    void truncation() {
        char out[8];
        std::memset(out, 0x6b, sizeof(out));
        int count = -1;
        auto size = smgpc_msl_snprintf(out, 4, "a%sbcdef%n", static_cast<const char *>(nullptr), &count);
        require(size == 6 && count == 6 && std::strcmp(out, "abc") == 0 && out[4] == 0x6b, "bounded sink truncates only output while n sees complete count");
        require(smgpc_msl_snprintf(nullptr, 0, "%s:%s", static_cast<const char *>(nullptr), "tail") == 5, "zero-size query reads no destination");
        char one = 'x';
        require(smgpc_msl_snprintf(&one, 1, "%s", "two") == 3 && one == 0, "size-one terminator");
        signed char c = -1;
        short s = -1;
        long l = -1;
        long long ll = -1;
        equal("abcd", "ab%s%hhn%hn%ln%llncd", "", &c, &s, &l, &ll);
        require(c == 2 && s == 2 && l == 2 && ll == 2, "all native n pointer widths preserve count");
    }
    void aliases() {
        char out[128];
        require(test_original_printf_alias(out, sizeof(out)) == 14 && std::strcmp(out, "/directory/007") == 0, "optimized std alias reaches MSL symbol");
        require(test_original_sprintf_alias(out) == 5 && std::strcmp(out, ":tail") == 0, "optimized global alias reaches MSL symbol");
        require(test_original_vsnprintf_alias(out, sizeof(out), "%s/%.2f", static_cast<const char *>(nullptr), 1.25) == 5 && std::strcmp(out, "/1.25") == 0, "optimized std va_list alias reaches MSL symbol");
        require(test_original_vsprintf_alias(out, "%s/%.2f", static_cast<const char *>(nullptr), 1.25) == 5 && std::strcmp(out, "/1.25") == 0, "optimized global va_list alias reaches MSL symbol");
    }
    void threads() {
        std::array<std::thread, 4> workers;
        std::atomic<bool> failed = false;
        for (int i = 0; i < 4; ++i)
            workers[i] = std::thread([&] {for(int n=0;n<100;++n){char out[32];if(smgpc_msl_snprintf(out,sizeof(out),"%s/%03d",static_cast<const char*>(nullptr),17)!=4||std::strcmp(out,"/017")!=0)failed=true;} });
        for (auto &t : workers)
            t.join();
        require(!failed, "formatting has no shared mutable parser or va_list state");
    }
}  // namespace
int main() {
    const std::array tests{std::pair{"null strings", null_strings}, std::pair{"MSL precision and padding", precision_padding}, std::pair{"Pascal and wide strings", pascal_wide}, std::pair{"MSL wide bytes", wide_bytes}, std::pair{"native numeric formatting", numeric}, std::pair{"mixed native varargs", mixed}, std::pair{"truncation and count", truncation}, std::pair{"original declaration aliases", aliases}, std::pair{"concurrent formatters", threads}};
    for (const auto &[label, test] : tests) {
        try {
            test();
            std::cout << "PASS " << label << '\n';
        } catch (const std::exception &e) {
            std::cerr << "FAIL " << label << ": " << e.what() << '\n';
            return 1;
        }
    }
}
