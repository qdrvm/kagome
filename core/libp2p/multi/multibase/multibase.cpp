/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <map>
#include <optional>

#include "libp2p/multi/multibase.hpp"
#include "libp2p/multi/multibase/codec.hpp"
#include "libp2p/multi/multibase/codecs/base16.hpp"
#include "libp2p/multi/multibase/codecs/base58.hpp"
#include "libp2p/multi/multibase/codecs/base64.hpp"

namespace {
  using namespace libp2p::multi;

  /**
   * Get the encoding by a character
   * @param ch of encoding
   * @return related encoding, if character stands for one of them, none
   * otherwise
   */
  std::optional<Multibase::Encoding> encodingByChar(char ch) {
    switch (ch) {
      case 'f':
        return Multibase::Encoding::kBase16;
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

  // all available codecs
  const std::map<Multibase::Encoding, std::shared_ptr<Codec>> codecs{
      {Multibase::Encoding::kBase16, std::make_shared<Base16Codec<false>>()},
      {Multibase::Encoding::kBase16Upper,
       std::make_shared<Base16Codec<true>>()},
      {Multibase::Encoding::kBase58, std::make_shared<Base58Codec>()},
      {Multibase::Encoding::kBase64, std::make_shared<Base64Codec>()}};
}  // namespace

namespace libp2p::multi {
  using namespace kagome::expected;

  Multibase::FactoryResult Multibase::createMultibaseFromEncoded(
      std::string_view encoded_data) {
    if (encoded_data.length() < 2) {
      return Error{"multibase must be at least 2 characters"};
    }

    auto encoding_base = encodingByChar(encoded_data.front());
    if (!encoding_base) {
      return Error{"base of encoding is either unsupported or does not exist"};
    }

    // can't immediately match and return Value(Multibase), as this is going to
    // call a private ctor inside a lambda, which is forbidden
    std::optional<ByteBuffer> decoded_data;
    std::string error;
    codecs.at(*encoding_base)
        ->decode(encoded_data)
        .match(
            [&decoded_data](const Value<ByteBuffer> bytes) mutable {
              decoded_data = bytes.value;
            },
            [&error](const Error<std::string> err) mutable {
              error = err.error;
            });

    if (!decoded_data) {
      return Error{error};
    }
    Multibase res{
        std::string{encoded_data}, std::move(*decoded_data), *encoding_base};
    return Value{std::make_unique<Multibase>(std::move(res))};
  }

  Multibase::FactoryResult Multibase::createMultibaseFromDecoded(
      const ByteBuffer &decoded_data, Encoding encoding_base) {
    if (decoded_data.size() == 0) {
      return Error{"no data provided"};
    }

    auto encoded_data = codecs.at(encoding_base)->encode(decoded_data);
    Multibase res{
        std::move(encoded_data), ByteBuffer{decoded_data}, encoding_base};
    return Value{std::make_unique<Multibase>(std::move(res))};
  }

  Multibase::Multibase(std::string &&encoded_data,
                       ByteBuffer &&decoded_data,
                       Encoding base)
      : encoded_data_{std::move(encoded_data)},
        decoded_data_{std::move(decoded_data)},
        base_{base} {}

  Multibase::Encoding Multibase::base() const {
    return base_;
  }

  std::string_view Multibase::encodedData() const {
    return encoded_data_;
  }

  const Multibase::ByteBuffer &Multibase::decodedData() const {
    return decoded_data_;
  }

  bool Multibase::operator==(const Multibase &rhs) const {
    return this->encoded_data_ == rhs.encoded_data_
        && this->decoded_data_ == rhs.decoded_data_ && this->base_ == rhs.base_;
  }

  bool Multibase::operator!=(const Multibase &rhs) const {
    return !(*this == rhs);
  }
}  // namespace libp2p::multi
