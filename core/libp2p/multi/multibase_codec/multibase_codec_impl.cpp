/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multibase_codec/multibase_codec_impl.hpp"

#include <optional>
#include <unordered_map>

#include "libp2p/multi/multibase_codec/codecs/base16.hpp"
#include "libp2p/multi/multibase_codec/codecs/base58.hpp"
#include "libp2p/multi/multibase_codec/codecs/base64.hpp"

namespace {
  using namespace libp2p::multi;          // NOLINT
  using namespace libp2p::multi::detail;  // NOLINT

  /**
   * Get the encoding by a character
   * @param ch of encoding
   * @return related encoding, if character stands for one of them, none
   * otherwise
   */
  constexpr std::optional<MultibaseCodec::Encoding> encodingByChar(char ch) {
    switch (ch) {
      case 'f':
        return MultibaseCodec::Encoding::kBase16Lower;
      case 'F':
        return MultibaseCodec::Encoding::kBase16Upper;
      case 'Z':
        return MultibaseCodec::Encoding::kBase58;
      case 'm':
        return MultibaseCodec::Encoding::kBase64;
      default:
        return {};
    }
  }

  struct CodecFunctions {
    using EncodeFuncType = decltype(encodeBase64);
    using DecodeFuncType = decltype(decodeBase64);

    EncodeFuncType *encode;
    DecodeFuncType *decode;
  };

  /// all available codec functions
  const std::unordered_map<MultibaseCodec::Encoding, CodecFunctions> codecs{
      {MultibaseCodec::Encoding::kBase16Upper,
       {&encodeBase16Upper, &decodeBase16Upper}},
      {MultibaseCodec::Encoding::kBase16Lower,
       {&encodeBase16Lower, &decodeBase16Lower}},
      {MultibaseCodec::Encoding::kBase58, {&encodeBase58, &decodeBase58}},
      {MultibaseCodec::Encoding::kBase64, {&encodeBase64, &decodeBase64}}};
}  // namespace

namespace libp2p::multi {
  using kagome::common::Buffer;
  using kagome::expected::Error;
  using kagome::expected::Result;
  using kagome::expected::Value;

  MultibaseCodecImpl::~MultibaseCodecImpl() = default;

  std::string MultibaseCodecImpl::encode(const Buffer &bytes,
                                         Encoding encoding) const {
    if (bytes.size() == 0) {
      return "";
    }

    return static_cast<char>(encoding) + codecs.at(encoding).encode(bytes);
  }

  Result<Buffer, std::string> MultibaseCodecImpl::decode(
      std::string_view string) const {
    if (string.length() < 2) {
      return Error{"encoded data must be at least 2 characters long"};
    }

    auto encoding_base = encodingByChar(string.front());
    if (!encoding_base) {
      return Error{"base of encoding is either unsupported or does not exist"};
    }

    return codecs.at(*encoding_base).decode(string.substr(1));
  }
}  // namespace libp2p::multi
