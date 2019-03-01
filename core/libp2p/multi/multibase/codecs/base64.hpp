/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BASE64_HPP
#define KAGOME_BASE64_HPP

#include <optional>

#include "libp2p/multi/multibase/codec.hpp"

namespace libp2p::multi {
  /**
   * Encode/decode to/from base64 format
   * Implementation is taken from https://github.com/boostorg/beast, as it is
   * not safe to use the Boost's code directly - it lies in 'detail' namespace,
   * which should not be touched externally
   */
  class Base64Codec : public Codec {
   public:
    std::string encode(const kagome::common::Buffer &bytes) const override;

    kagome::expected::Result<kagome::common::Buffer, std::string> decode(
        std::string_view string) const override;

   private:
    std::string encodeMediator(uint8_t const *data, size_t len) const;
    size_t encodeImpl(void *dest, void const *src, size_t len) const;

    std::optional<std::string> decodeMediator(std::string_view data) const;
    std::pair<std::size_t, std::size_t> decodeImpl(void *dest,
                                                   char const *src,
                                                   size_t len) const;
  };
}  // namespace libp2p::multi

#endif  // KAGOME_BASE64_HPP
