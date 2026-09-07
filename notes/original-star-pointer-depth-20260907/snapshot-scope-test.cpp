#include <aurora/depth_snapshot.hpp>
#include <cassert>
#include <thread>
int main() {
    assert(aurora::selected_depth_snapshot == 0);
    {
        aurora::ScopedDepthSnapshotRead first(19);
        std::thread other([] { assert(aurora::selected_depth_snapshot == 0); });
        other.join();
        try {
            aurora::ScopedDepthSnapshotRead second(23);
            assert(aurora::selected_depth_snapshot == 23);
            throw 1;
        } catch (int) {}
        assert(aurora::selected_depth_snapshot == 19);
    }
    assert(aurora::selected_depth_snapshot == 0);
}
