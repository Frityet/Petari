#pragma once

#define SMGPC_INTEGRATION_BEGIN
#define SMGPC_INTEGRATION_END

#define SMGPC_STUB_JOIN_IMPL(a, b) a##b
#define SMGPC_STUB_JOIN(a, b) SMGPC_STUB_JOIN_IMPL(a, b)
#define SMGPC_STUB(symbol) [[maybe_unused]] static constexpr const char *SMGPC_STUB_JOIN(sSmgPcStub_, __LINE__) = #symbol
