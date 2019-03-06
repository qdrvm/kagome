/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multibase/multibase_impl.hpp"

#include <optional>
#include <unordered_map>

#include "libp2p/multi/multibase/codecs/base16.hpp"
#include "libp2p/multi/multibase/codecs/base58.hpp"
#include "libp2p/multi/multibase/codecs/base64.hpp"

namespace {
  using namespace libp2p::multi;
  using namespace libp2p::multi::detail;

  /**
   * Get the encoding by a character
   * @param ch of encoding
   * @return related encoding, if character stands for one of them, none
   * otherwise
   */
  std::optional<Multibase::Encoding> encodingByChar(char ch) {
    switch (ch) {
      case 'f':
        return Multibase::Encoding::kBase16Lower;
      case 'F':
        return Multibase::Encoding::kBase16Upper;
      case 'Z':
        return Multibase::Encoding::kBase58;
      case 'm':
        return Multibase::Encoding::kBase64;
      default:
        return {};
    }
  }

  using EncodeFuncType = decltype(encodeBase64);
  using DecodeFuncType = decltype(decodeBase64);

  /// all available codec functions
  const std::unordered_map<Multibase::Encoding,
                           std::pair<EncodeFuncType *, DecodeFuncType *>>
      codecs{{Multibase::Encoding::kBase16Upper,
              {&encodeBase16Upper, &decodeBase16Upper}},
             {Multibase::Encoding::kBase16Lower,
              {&encodeBase16Lower, &decodeBase16Lower}},
             {Multibase::Encoding::kBase58, {&encodeBase58, &decodeBase58}},
             {Multibase::Encoding::kBase64, {&encodeBase64, &decodeBase64}}};
}  // namespace

namespace libp2p::multi {
  using namespace kagome::expected;
  using namespace kagome::common;

  std::string MultibaseImpl::encode(const Buffer &bytes,
                                    Encoding encoding) const {
    if (bytes.size() == 0) {
      return "no data provided";
    }

    return static_cast<char>(encoding) + codecs.at(encoding).first(bytes);
  }

  Result<Buffer, std::string> MultibaseImpl::decode(
      std::string_view string) const {
    if (string.length() < 2) {
      return Error{"encoded data must be at least 2 characters long"};
    }

    auto encoding_base = encodingByChar(string.front());
    if (!encoding_base) {
      return Error{"base of encoding is either unsupported or does not exist"};
    }

    std::optional<Buffer> decoded_data;
    std::string error;
    codecs.at(*encoding_base)
        .second(string.substr(1))  // cut the prefix
        .match(
            [&decoded_data](const Value<Buffer> &bytes) mutable {
              decoded_data = bytes.value;
            },
            [&error](const Error<std::string> &err) mutable {
              error = err.error;
            });

    if (!decoded_data) {
      return Error{std::move(error)};
    }
    return Value{std::move(*decoded_data)};
  }
}  // namespace libp2p::multi
