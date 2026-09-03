#include "JSystem/JKernel/JKRSolidHeap.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <new>
#include <stdexcept>

namespace {
    void require(bool value, const char* message) {
        if (!value) throw std::runtime_error(message);
    }

    struct Fixture {
        std::array<int, 3> destroyed{};
        alignas(64) std::array<u8, 32768> arena{};
        JKRSolidHeap root{arena.data(), u32(arena.size()), nullptr, false};

        Fixture() {
            require(JKRHeap::sRootHeap == nullptr, "fixture requires no active runtime");
            JKRHeap::sRootHeap = &root;
        }
        ~Fixture() {
            // The original base destructor consults the still-live root. Run
            // it before clearing the global pointers, including on exceptions.
            root.dispose();
        }
    };

    struct Tracked final : JKRDisposer {
        int& count;
        explicit Tracked(int& value) : count(value) {}
        ~Tracked() override { ++count; }
    };

    Tracked* tracked(JKRHeap& heap, int& counter, int alignment = 16) {
        void* memory = heap.alloc(sizeof(Tracked), alignment);
        require(memory != nullptr, "tracking allocation");
        return new (memory) Tracked(counter);
    }

    void run_fixture(void (*test)(Fixture&)) {
        { Fixture fixture; test(fixture); }
        JKRHeap::sRootHeap = nullptr;
        JKRHeap::sCurrentHeap = nullptr;
        JKRHeap::sSystemHeap = nullptr;
    }

    void test_alignment_and_reset(Fixture& f) {
        const u32 capacity = f.root.getFreeSize();
        void* head = f.root.alloc(17, 64);
        void* tail = f.root.alloc(23, -128);
        require(head && tail, "both allocation directions");
        require(reinterpret_cast<std::uintptr_t>(head) % 64 == 0 &&
                    reinterpret_cast<std::uintptr_t>(tail) % 128 == 0,
                "native addresses remain aligned without truncation");
        require(head < tail && f.root.check(), "head and tail preserve free interval");
        const u32 remaining = f.root.getFreeSize();
        f.root.free(head);
        require(f.root.getFreeSize() == remaining, "retail SolidHeap individual free is a no-op");
        f.root.freeTail();
        require(f.root.getFreeSize() > remaining && f.root.check(), "tail release restores available interval");
        f.root.freeAll();
        require(f.root.getFreeSize() == capacity && f.root.check(), "bulk release restores all capacity");
    }

    void test_disposer_ranges(Fixture& f) {
        auto& [a, b, c] = f.destroyed;
        Tracked* first = tracked(f.root, a);
        Tracked* middle = tracked(f.root, b);
        Tracked* last = tracked(f.root, c, -32);
        require(first->mHeap == &f.root && last->mHeap == &f.root &&
                    f.root.mDisposerList.getNumLinks() == 3, "original disposer registers in owning heap");
        f.root.dispose(middle, u32(sizeof(Tracked)));
        require(a == 0 && b == 1 && c == 0 && f.root.mDisposerList.getNumLinks() == 2,
                "native-width range disposes only middle object");
        f.root.freeTail();
        require(a == 0 && b == 1 && c == 1, "tail release invokes actual virtual disposer destructor");
        f.root.freeAll();
        require(a == 1 && b == 1 && c == 1 && f.root.mDisposerList.getNumLinks() == 0,
                "bulk release destroys each remaining object exactly once");
    }

    void test_nested_heaps(Fixture& f) {
        JKRSolidHeap* child = JKRSolidHeap::create(4096, &f.root, false);
        require(child && child->getParent() == &f.root, "original child heap construction");
        require(JKRHeap::getCurrentHeap() == child && JKRHeap::getSystemHeap() == child,
                "first child replaces root current and system selection");
        JKRSolidHeap* grandchild = JKRSolidHeap::create(1024, child, false);
        require(grandchild && grandchild->getParent() == child, "original grandchild construction");
        int& destroyed = f.destroyed[0];
        Tracked* value = tracked(*grandchild, destroyed);
        require(JKRHeap::findFromRoot(value) == grandchild && child->find(value) == grandchild,
                "ownership lookup selects deepest containing heap");
        grandchild->becomeCurrentHeap();
        grandchild->destroy();
        require(destroyed == 1 && child->mChildTree.getNumChildren() == 0,
                "destroy runs owned disposers and unlinks child tree");
        require(JKRHeap::getCurrentHeap() == child, "current heap returns to first root child");
        child->destroy();
        require(f.root.mChildTree.getNumChildren() == 0 && f.root.mDisposerList.getNumLinks() == 0,
                "heap is removed from both independent ownership lists");
        require(JKRHeap::getCurrentHeap() == &f.root && JKRHeap::getSystemHeap() == &f.root,
                "last child restores root current and system heaps");
    }

    void test_external_child(Fixture& f) {
        alignas(64) std::array<u8, 2048> external{};
        {
            JKRSolidHeap child(external.data(), external.size(), &f.root, false);
            int& destroyed = f.destroyed[0];
            Tracked* value = tracked(child, destroyed);
            require(JKRHeap::findFromRoot(value) == &child,
                    "findAllHeap discovers child outside root address interval");
            child.dispose();
            require(destroyed == 1, "external child disposer lifecycle");
        }
        require(f.root.mChildTree.getNumChildren() == 0, "external child tree unlink");
    }

    void test_list_transfer(Fixture&) {
        int values[3]{};
        JSULink<int> a(&values[0]), b(&values[1]), c(&values[2]);
        JSUList<int> first, second;
        require(first.append(&a) && first.append(&b) && first.prepend(&c), "original intrusive append and prepend");
        require(first.getFirst() == &c && first.getLast() == &b && first.getNumLinks() == 3,
                "list order and count");
        require(second.append(&a) && first.getNumLinks() == 2 && a.getSupervisor() == &second,
                "append transfers ownership between lists");
        require(second.insert(&a, &b) && second.getFirst() == &b && first.getNumLinks() == 1,
                "insert transfers ownership before another node");
        require(!first.remove(&a) && second.remove(&a) && a.getSupervisor() == nullptr,
                "removal validates exact owning list");
    }
}

int main() {
    try {
        run_fixture(test_alignment_and_reset);
        run_fixture(test_disposer_ranges);
        run_fixture(test_nested_heaps);
        run_fixture(test_external_child);
        run_fixture(test_list_transfer);
        std::cout << "[pass] 5 original JKR heap/list groups\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] " << error.what() << '\n';
        return 1;
    }
}
