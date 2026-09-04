#include <functional.hpp>
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }
    struct Referent {
        int value;
        explicit Referent(int input) : value(input) {}
        Referent(const Referent&) = delete;
    };
    struct Receiver {
        int observed = 0;
        const Referent* address = nullptr;
        void scalar(int value) { observed = value; }
        int scalar_const(int value) const { return value; }
        void reference(const Referent& value) { observed = value.value; address = &value; }
        int reference_const(const Referent& value) const noexcept { return value.value; }
        void scalar_noexcept(int value) noexcept { observed = value; }
    };
    auto returned_callback() {
        long value = -1;
        return std::bind2nd(std::mem_func(&Receiver::scalar), value);
    }
}
int main() {
    try {
        Receiver receiver;
        long source = 27;
        auto scalar = std::bind2nd(std::mem_func(&Receiver::scalar), source);
        static_assert(std::is_same_v<decltype(scalar.value), int>);
        source = 99;
        scalar(&receiver);
        require(receiver.observed == 27, "value argument is copied and converted at binding");
        auto returned = returned_callback();
        returned(&receiver);
        require(receiver.observed == -1, "returned callback does not borrow a dead scalar");
        auto temporary = std::bind2nd(std::mem_func(&Receiver::scalar_const), -1L);
        require(temporary(&receiver) == -1, "temporary value and const member remain valid");
        Referent object(7);
        auto reference = std::bind2nd(std::mem_func(&Receiver::reference), object);
        static_assert(std::is_same_v<decltype(reference.value), const Referent&>);
        object.value = 23;
        reference(&receiver);
        require(receiver.observed == 23 && receiver.address == &object, "reference argument retains exact live identity without copying");
        auto const_reference = std::bind2nd(std::mem_func(&Receiver::reference_const), object);
        require(const_reference(&receiver) == 23, "const noexcept reference member dispatch");
        auto no_throw = std::bind2nd(std::mem_func(&Receiver::scalar_noexcept), -7L);
        no_throw(&receiver);
        require(receiver.observed == -7, "noexcept scalar member dispatch");
        std::cout << "PASS MSL bind2nd declared argument storage, scalar lifetime/conversion and reference identity\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
