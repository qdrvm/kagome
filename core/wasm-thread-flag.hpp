#pragma once

inline auto &wasmThreadFlag() {
  static bool x = false;
  return x;
}
