/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
/*
   Portions from http://www.adp-gmbh.ch/cpp/common/base64.html
   Copyright notice:

   base64.cpp and base64.h

   Copyright (C) 2004-2008 Rene Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   Rene Nyffenegger rene.nyffenegger@adp-gmbh.ch
*/

#include <regex>

#include "libp2p/multi/multibase/codecs/base64.hpp"

namespace {

  const std::string_view alphabet{
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};

  constexpr std::array<signed char, 256> inverse_table{
      // clang-format off
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //   0-15
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //  16-31
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, //  32-47
      52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, //  48-63
      -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, //  64-79
      15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, //  80-95
      -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, //  96-111
      41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, // 112-127
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 128-143
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 144-159
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 160-175
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 176-191
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 192-207
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 208-223
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 224-239
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1  // 240-255
      // clang-format on
  };

  /// Returns max bytes needed to decode a base64 string
  inline std::size_t constexpr decodedSize(std::size_t n) {
    return n / 4 * 3;  // requires n&3==0, smaller
  }

  const std::regex base64_regex{"^[a-zA-Z0-9\\+/]*={0,2}$"};
  /**
   * Valid string must have multiple of 4 size and contain specific symbols
   * @param string to be checked
   * @return true, if the string is valid base64 string, false otherwise
   */
  bool isValidBase64(std::string_view string) {
    return string.size() % 4 == 0
        && std::regex_match(string.data(), base64_regex);
  }
}  // namespace

namespace libp2p::multi {
  using namespace kagome::expected;

  std::string Base64Codec::encode(const kagome::common::Buffer &bytes) const {
    std::string dest;
    dest.resize(encodeImpl(dest, bytes));
    return dest;
  }

  Result<kagome::common::Buffer, std::string> Base64Codec::decode(
      std::string_view string) const {
    auto error_msg = [string] {
      return Error{"string '" + std::string{string}
                   + "' is not a valid base64 encoded string"};
    };
    if (!isValidBase64(string)) {
      return error_msg();
    }

    auto decoded_bytes = decodeImpl(string);

    if (!decoded_bytes) {
      return error_msg();
    }
    return Value{kagome::common::Buffer{std::move(*decoded_bytes)}};
  }

  size_t Base64Codec::encodeImpl(std::string &out,
                                 const kagome::common::Buffer &bytes) const {
    auto len = bytes.size();
    const auto tab = alphabet;
    size_t bytes_pos = 0, decoded_size = 0;

    for (auto n = len / 3; n--;) {
      out += tab[(bytes[bytes_pos + 0] & 0xfc) >> 2];
      out += tab[((bytes[bytes_pos + 0] & 0x03) << 4)
                 + ((bytes[bytes_pos + 1] & 0xf0) >> 4)];
      out += tab[((bytes[bytes_pos + 2] & 0xc0) >> 6)
                 + ((bytes[bytes_pos + 1] & 0x0f) << 2)];
      out += tab[bytes[bytes_pos + 2] & 0x3f];
      bytes_pos += 3;
      decoded_size += 4;
    }

    switch (len % 3) {
      case 2:
        out += tab[(bytes[bytes_pos + 0] & 0xfc) >> 2];
        out += tab[((bytes[bytes_pos + 0] & 0x03) << 4)
                   + ((bytes[bytes_pos + 1] & 0xf0) >> 4)];
        out += tab[(bytes[bytes_pos + 1] & 0x0f) << 2];
        out += '=';
        decoded_size += 4;
        break;

      case 1:
        out += tab[(bytes[bytes_pos + 0] & 0xfc) >> 2];
        out += tab[((bytes[bytes_pos + 0] & 0x03) << 4)];
        out += '=';
        out += '=';
        decoded_size += 4;
        break;

      default:
        break;
    }

    return decoded_size;
  }

  std::optional<std::vector<uint8_t>> Base64Codec::decodeImpl(
      std::string_view src) const {
    std::vector<uint8_t> out(decodedSize(src.size()));

    std::vector<unsigned char> c3(3), c4(4);
    int i = 0, j = 0;
    size_t in_pos = 0, len = src.size(), bytes_pos = 0;

    while (len-- && src[in_pos] != '=') {
      auto const v = inverse_table.at(src[in_pos]);
      if (v == -1)
        return {};
      ++in_pos;
      c4[i] = v;
      if (++i == 4) {
        c3[0] = (c4[0] << 2) + ((c4[1] & 0x30) >> 4);
        c3[1] = ((c4[1] & 0xf) << 4) + ((c4[2] & 0x3c) >> 2);
        c3[2] = ((c4[2] & 0x3) << 6) + c4[3];

        for (i = 0; i < 3; i++) {
          out[bytes_pos] = c3[i];
          ++bytes_pos;
        }
        i = 0;
      }
    }

    if (i) {
      c3[0] = (c4[0] << 2) + ((c4[1] & 0x30) >> 4);
      c3[1] = ((c4[1] & 0xf) << 4) + ((c4[2] & 0x3c) >> 2);
      c3[2] = ((c4[2] & 0x3) << 6) + c4[3];

      for (j = 0; j < i - 1; j++) {
        out[bytes_pos] = c3[j];
        ++bytes_pos;
      }
    }
    out.resize(bytes_pos);
    return out;
  }
}  // namespace libp2p::multi
