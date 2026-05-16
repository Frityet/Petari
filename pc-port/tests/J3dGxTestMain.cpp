#include "TestSuites.hpp"
#include "TestSupport.hpp"

int main() {
    return smgpc::tests::run_named_test_suite("j3d/gx", smgpc::tests::run_j3d_gx_tests);
}
