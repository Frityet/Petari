#pragma once

// The host port does not emulate JKRHeap disposer-list ownership yet. Keep the
// SDK ownership type present so original Game classes retain their inheritance
// surface; unsupported heap registration remains absent.
class JKRDisposer {
public:
    JKRDisposer() = default;
    virtual ~JKRDisposer() = default;
};
