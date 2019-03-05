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
    /**
     * Actual implementation of the encoding
     * @param dest, where to put encoded chars
     * @param bytes to be encoded
     * @return how much chars are in the resulting string
     */
    size_t encodeImpl(std::string &dest,
                      const kagome::common::Buffer &bytes) const;

    /**
     * Actual implementation of the decoding
     * @param src to be decoded
     * @return bytes, if decoding went successful, none otherwise
     */
    std::optional<std::vector<uint8_t>> decodeImpl(std::string_view src) const;
  };
}  // namespace libp2p::multi

#endif  // KAGOME_BASE64_HPP
