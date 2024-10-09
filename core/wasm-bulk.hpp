#pragma once

inline auto &wasmBulk() {
  static auto x = false;
  return x;
}
