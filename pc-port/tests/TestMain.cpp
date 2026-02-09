#include "tests/TestHarness.hpp"

#include <exception>
#include <iostream>
#include <optional>
#include <string>

int main(int argc, char** argv) {
    std::optional<std::string> caseFilter;
    bool listTests = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--list-tests") {
            listTests = true;
        } else if (arg == "--case") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for --case\n";
                return 2;
            }
            caseFilter = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << arg << '\n';
            return 2;
        }
    }

    if (listTests) {
        for (const auto& test : pcport::test::Registry()) {
            std::cout << test.name << '\n';
        }
        return 0;
    }

    int failures = 0;
    bool matchedCase = false;

    for (const auto& test : pcport::test::Registry()) {
        if (caseFilter.has_value() && test.name != *caseFilter) {
            continue;
        }

        matchedCase = true;

        try {
            test.fn();
            std::cout << "[PASS] " << test.name << '\n';
        } catch (const std::exception& ex) {
            ++failures;
            std::cout << "[FAIL] " << test.name << " :: " << ex.what() << '\n';
        } catch (...) {
            ++failures;
            std::cout << "[FAIL] " << test.name << " :: unknown exception\n";
        }
    }

    if (caseFilter.has_value() && !matchedCase) {
        std::cout << "Unknown test case: " << *caseFilter << '\n';
        return 2;
    }

    if (failures > 0) {
        std::cout << "Failures: " << failures << '\n';
        return 1;
    }

    if (caseFilter.has_value()) {
        std::cout << "Case passed: " << *caseFilter << '\n';
    } else {
        std::cout << "All tests passed: " << pcport::test::Registry().size() << '\n';
    }
    return 0;
}
