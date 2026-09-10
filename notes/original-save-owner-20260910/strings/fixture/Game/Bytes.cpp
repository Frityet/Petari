#include <cstring>
#define WIDE(x) u##x
#define PART "共"
const char *raw = R"tag(表\u5171
共)tag";
static_assert(sizeof("共") == 3);
static_assert('共' == 0x8ba4);
static_assert(sizeof(u8"共") == 4);
static_assert(sizeof(u"日" PART) == 6);
static_assert(WIDE("共")[0] == 0x5171);
int main() {
 return std::strcmp("\u5171", "\213\244") ||
        std::strcmp("\u{5171}", "\213\244") ||
        std::strcmp("\N{CJK UNIFIED IDEOGRAPH-5171}", "\213\244") ||
        std::strcmp(raw, "\225\134\\u5171\n\213\244");
}
