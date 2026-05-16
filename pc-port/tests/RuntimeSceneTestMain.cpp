#include "TestSuites.hpp"
#include "TestSupport.hpp"

int main() {
    return smgpc::tests::run_named_test_suite("runtime/scene", smgpc::tests::run_runtime_scene_tests);
}
