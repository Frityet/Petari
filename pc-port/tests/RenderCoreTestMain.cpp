#include "TestSuites.hpp"
#include "TestSupport.hpp"

int main() {
    return smgpc::tests::run_named_test_suite("render/core", smgpc::tests::run_render_core_tests);
}
