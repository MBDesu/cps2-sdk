#pragma once
#ifndef MODULE_LIB_BYTE_H
#define MODULE_LIB_BYTE_H

#include "../globals.h"

#include <type_traits>

template <typename T> constexpr T MAKE_MASK(const u8 start, const u8 size) {
  return ((T(1) << size) - 1) << start;
}

template <typename T>
constexpr T BITS(const T value, const u8 start, const u8 size) {
  return (value >> start) & MAKE_MASK<T>(0, size);
}

template <typename T> constexpr T BIT(const T value, const u8 pos) {
  return BITS(value, pos, 1);
}

template <typename T, u8 bit> struct Bit {
  T &source;

  operator bool() const { return BIT(source, bit); }
  Bit &operator=(bool value) {
    constexpr T mask = MAKE_MASK<T>(bit, 1);
    if (value)
      source |= mask;
    else
      source &= ~mask;

    return *this;
  }
};

template <typename T, u8 start, u8 size> struct Bits {
  T &source;
  operator T() const { return BITS(source, start, size); }
  Bits &operator=(T value) {
    constexpr T mask = MAKE_MASK<T>(start, size);
    source = (source & ~mask) | ((value & MAKE_MASK<T>(0, size)) << start);
    return *this;
  }
};

template <int N, typename T, typename... Bits>
constexpr auto cps2_bitswap(T val, Bits... bits) noexcept {
  static_assert(sizeof...(Bits) == N, "Number of bit indices must match N");
  using Out = std::conditional_t<
      (N <= 8), u8,
      std::conditional_t<(N <= 16), u16,
                         std::conditional_t<(N <= 32), u32, u64>>>;

  Out res = 0;
  int shift = N - 1;
  ((res |= static_cast<Out>(BIT(val, bits)) << shift--), ...);
  return res;
};

#endif // MODULE_LIB_BYTE_H
