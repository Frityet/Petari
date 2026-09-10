#pragma once
#define STRINGIFY_INNER(x) #x
#define STRINGIFY(x) STRINGIFY_INNER(x)
#define PREFIX_JOIN(a, b) a##b
#define GAME_NARROW "共"
inline const char *game_plain() { return "共"; }
inline const char *game_stringized() { return STRINGIFY("共"); }
inline const char16_t *game_wide_prefix() { return PREFIX_JOIN(u, "共"); }
inline const char16_t *game_wide_concat() { return u"日" GAME_NARROW; }
inline const char8_t *game_utf8_prefix() { return PREFIX_JOIN(u8, "共"); }
inline const char *game_path() { return __FILE__; }
inline const char *game_macro() { return GAME_NARROW; }
inline const char *game_escaped() { return "表ソ十"; }
