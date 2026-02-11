#pragma once

#include <exception>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace smgpc::test {

using TestFn = std::function<void()>;

struct TestCase {
    std::string name;
    TestFn fn;
};

inline std::vector<TestCase>& Registry() {
    static std::vector<TestCase> tests;
    return tests;
}

inline void Register(std::string name, TestFn fn) {
    Registry().push_back(TestCase{std::move(name), std::move(fn)});
}

inline std::runtime_error MakeFailure(const char* expr, const char* file, int line) {
    std::ostringstream message;
    message << file << ':' << line << " assertion failed: " << expr;
    return std::runtime_error(message.str());
}

template <typename A, typename B>
inline std::runtime_error MakeEqFailure(const A& lhs, const B& rhs, const char* lhsExpr, const char* rhsExpr, const char* file, int line) {
    std::ostringstream message;
    message << file << ':' << line << " assertion failed: " << lhsExpr << " == " << rhsExpr << " (" << lhs << " vs " << rhs << ')';
    return std::runtime_error(message.str());
}

}  // namespace smgpc::test

#define $pc_port_concat_impl(a, b) a##b
#define $pc_port_concat(a, b) $pc_port_concat_impl(a, b)
#define $test(name) \
    static void $pc_port_concat(_pc_port_test_fn_, __LINE__)(); \
    namespace { \
    const bool $pc_port_concat(_pc_port_test_reg_, __LINE__) = [] { ::smgpc::test::Register((name), &$pc_port_concat(_pc_port_test_fn_, __LINE__)); return true; }(); \
    } \
    static void $pc_port_concat(_pc_port_test_fn_, __LINE__)()

#define $pc_port_require(expr)                                                                                                              \
    do {                                                                                                                                    \
        if (not (expr)) {                                                                                                                   \
            throw ::smgpc::test::MakeFailure(#expr, __FILE__, __LINE__);                                                                  \
        }                                                                                                                                   \
    } while (false)

#define $pc_port_require_eq(lhs, rhs)                                                                                                       \
    do {                                                                                                                                    \
        const auto& lhsValue = (lhs);                                                                                                       \
        const auto& rhsValue = (rhs);                                                                                                       \
        if (not (lhsValue == rhsValue)) {                                                                                                   \
            throw ::smgpc::test::MakeEqFailure(lhsValue, rhsValue, #lhs, #rhs, __FILE__, __LINE__);                                      \
        }                                                                                                                                   \
    } while (false)
