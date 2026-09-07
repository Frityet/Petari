#pragma once

namespace std {
    template < class InputIterator, class Function >
    inline Function for_each_array(InputIterator* first, InputIterator* last, Function f) {
        for (; first != last; ++first) {
            f(first);
        }

        return f;
    }

}
