/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UTILS_JSON_UNQUOTE_HPP
#define KAGOME_UTILS_JSON_UNQUOTE_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#define RAPIDJSON_NO_SIZETYPEDEFINE
namespace rapidjson {
  typedef ::std::size_t SizeType;
}  // namespace rapidjson

#include <rapidjson/reader.h>

namespace kagome {
  inline std::optional<std::string> jsonUnquote(std::string_view json) {
    struct Handler : rapidjson::BaseReaderHandler<rapidjson::UTF8<>, Handler> {
      bool Default() const {
        return false;
      }
      bool String(const char *ptr, size_t size, bool) {
        str.assign(ptr, size);
        return true;
      }

      std::string str;
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

#endif  // KAGOME_UTILS_JSON_UNQUOTE_HPP
