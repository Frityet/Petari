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

#define $pc_port_test(name)                                                                                                                \
    static void name();                                                                                                                     \
    namespace {                                                                                                                             \
    struct name##_Registration final {                                                                                                      \
        name##_Registration() { ::smgpc::test::Register(#name, &name); }                                                                  \
    } name##_registration;                                                                                                                  \
    }                                                                                                                                       \
    static void name()

#define $pc_port_require(expr)                                                                                                              \
    do {                                                                                                                                    \
        if (!(expr)) {                                                                                                                      \
            throw ::smgpc::test::MakeFailure(#expr, __FILE__, __LINE__);                                                                  \
        }                                                                                                                                   \
    } while (false)

#define $pc_port_require_eq(lhs, rhs)                                                                                                       \
    do {                                                                                                                                    \
        const auto& lhsValue = (lhs);                                                                                                       \
        const auto& rhsValue = (rhs);                                                                                                       \
        if (!(lhsValue == rhsValue)) {                                                                                                      \
            throw ::smgpc::test::MakeEqFailure(lhsValue, rhsValue, #lhs, #rhs, __FILE__, __LINE__);                                      \
        }                                                                                                                                   \
    } while (false)
