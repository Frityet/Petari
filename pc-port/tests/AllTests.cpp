#include "TestSuites.hpp"
#include "TestSupport.hpp"

int main() try {
    smgpc::tests::run_resource_layout_tests();
    smgpc::tests::run_j3d_gx_tests();
    smgpc::tests::run_runtime_scene_tests();
    smgpc::tests::run_render_core_tests();
    std::cout << "all tests passed\n";
    return 0;
} catch (const std::exception &e) {
    std::cerr << "all tests failed: " << e.what() << '\n';
    return 1;
}
