#include <functional.hpp>
#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>

struct Item {
    bool valid;
    int visits = 0;
    bool isValid() const noexcept { return valid; }
    bool visit() { ++visits; return valid; }
};
int main() {
    std::array<Item,3> items{{{true},{false},{true}}};
    const auto invalid = std::not1(std::mem_fun_ref(&Item::isValid));
    assert(std::find_if(items.begin(),items.end(),invalid) == items.begin()+1);
    const auto& const_items = items;
    assert(std::find_if(const_items.begin(),const_items.end(),invalid) == const_items.begin()+1);
    const auto not_visited_valid = std::not1(std::mem_fun_ref(&Item::visit));
    assert(std::find_if(items.begin(),items.end(),not_visited_valid) == items.begin()+1);
    assert(items[0].visits==1 && items[1].visits==1 && items[2].visits==0);
    items[1].valid=true;
    assert(std::find_if(items.begin(),items.end(),invalid)==items.end());
    assert(std::find_if(items.begin(),items.begin(),invalid)==items.begin());
    std::cout << "legacy_mem_fun_ref_not1=pass mutable_reference_identity=pass const_noexcept_member=pass empty_and_exhausted_ranges=pass\n";
}
