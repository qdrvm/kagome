/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>
#include <fstream>
#include <system_error>

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
  bool readFile(Out &out,
                const std::filesystem::path &path,
                std::error_code &ec) {
    std::ifstream file{path, std::ios::binary | std::ios::ate};
    if (not file.good()) {
      ec = std::error_code{errno, std::system_category()};
      out.clear();
      return false;
    }
    out.resize(file.tellg());
    file.seekg(0);
    file.read(reinterpret_cast<char *>(out.data()), out.size());
    if (not file.good()) {
      ec = std::error_code{errno, std::system_category()};
      out.clear();
      return false;
    }
    ec = {};
    return true;
  }

  template <ByteContainer Out>
  bool readFile(Out &out, const std::filesystem::path &path) {
    [[maybe_unused]] std::error_code ec;
    return readFile(out, path, ec);
  }

}  // namespace kagome
