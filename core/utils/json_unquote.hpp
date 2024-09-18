/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#define RAPIDJSON_NO_SIZETYPEDEFINE
namespace rapidjson {
  using SizeType = ::std::size_t;
}  // namespace rapidjson

#include <rapidjson/reader.h>

namespace kagome {

  // Remove outer quotes and parse escape sequences in a JSON string
  template <typename StringType = std::string>
  inline std::optional<StringType> jsonUnquote(std::string_view json) {
    struct Handler : rapidjson::BaseReaderHandler<rapidjson::UTF8<>, Handler> {
      bool Default() const {
        return false;
      }
      bool String(const char *ptr, size_t size, bool) {
        str.resize(size);
        std::copy_n(ptr, size, std::begin(str));
        return true;
      }

      StringType str;
    };
    rapidjson::MemoryStream stream{json.data(), json.size()};
    rapidjson::Reader reader;
    Handler handler;
    if (reader.Parse(stream, handler)) {
      return std::move(handler.str);
    }
    return std::nullopt;
  }
}  // namespace kagome
