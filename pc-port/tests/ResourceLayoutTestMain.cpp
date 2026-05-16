#include "TestSuites.hpp"
#include "TestSupport.hpp"

int main() {
    return smgpc::tests::run_named_test_suite("resource/layout", smgpc::tests::run_resource_layout_tests);
}
