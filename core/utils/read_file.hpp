/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>
#include <fstream>
#include <system_error>

#include <qtils/outcome.hpp>

namespace kagome {

  template <typename T>
  concept StandardLayoutPointer =
      std::is_standard_layout_v<std::remove_pointer_t<T>>;

  template <typename T>
  concept ByteContainer =  //
      requires(T t, std::streampos pos) {
        { t.data() } -> StandardLayoutPointer;
        { t.size() } -> std::convertible_to<std::streamsize>;
        { t.resize(pos) };
        { t.clear() };
      };

  template <ByteContainer Out>
  outcome::result<void> readFile(Out &out, const std::filesystem::path &path) {
    std::ifstream file{path, std::ios::binary | std::ios::ate};
    if (not file.good()) {
      out.clear();
      return std::errc{errno};
    }
    out.resize(file.tellg());
    file.seekg(0);
    file.read(reinterpret_cast<char *>(out.data()), out.size());
    if (not file.good()) {
      out.clear();
      return std::errc{errno};
    }
    return outcome::success();
  }
}  // namespace kagome
