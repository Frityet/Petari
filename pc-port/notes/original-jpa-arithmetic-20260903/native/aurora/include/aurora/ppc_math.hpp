#pragma once

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

namespace aurora::ppc {
// Gekko fctiwz's integer word result. Emulated FPSCR/CR state is outside this API.
inline std::int32_t truncate_s32(double value) {
  if (std::isnan(value) || value < -2147483648.0) {
    return std::numeric_limits<std::int32_t>::min();
  }
  if (value >= 2147483648.0) {
    return std::numeric_limits<std::int32_t>::max();
  }
  return static_cast<std::int32_t>(value);
}

// sth preserves only the low halfword; extsh interprets that halfword as signed.
inline std::int16_t narrow_s16(std::uint32_t value) {
  return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(value));
}
inline std::uint16_t truncate_u16(double value) {
  return static_cast<std::uint16_t>(static_cast<std::uint32_t>(truncate_s32(value)));
}
inline std::int16_t truncate_s16(double value) {
  return narrow_s16(static_cast<std::uint32_t>(truncate_s32(value)));
}

// Gekko divw's result for zero divisors and signed overflow follows the dividend sign.
inline std::int32_t divide_s32(std::int32_t numerator, std::int32_t denominator) {
  if (denominator == 0 || (numerator == std::numeric_limits<std::int32_t>::min() && denominator == -1)) {
    return numerator < 0 ? -1 : 0;
  }
  return numerator / denominator;
}

// slw consumes the low six count bits; bit 5 clears the result.
inline std::int32_t shift_left_s32(std::int32_t value, std::uint32_t count) {
  const std::uint32_t shifted = (count & 0x20U) ? 0U : static_cast<std::uint32_t>(value) << (count & 0x1fU);
  return std::bit_cast<std::int32_t>(shifted);
}
} // namespace aurora::ppc
