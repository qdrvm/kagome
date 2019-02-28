/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CODEC_HPP
#define KAGOME_CODEC_HPP

#include <string>
#include <string_view>

#include "common/buffer.hpp"
#include "common/result.hpp"

namespace libp2p::multi {
  /**
   * Encode and decode bytes or encoded strings based on the implementation
   */
  class Codec {
   public:
    /**
     * Encode the incoming bytes
     * @param bytes to be encoded
     * @return encoded string
     */
    virtual std::string encode(const kagome::common::Buffer &bytes) const = 0;

    /**
     * Decode the incoming string
     * @param string to be decoded
     * @return bytes, if decoding was successful, error otherwise
     */
    virtual kagome::expected::Result<kagome::common::Buffer, std::string>
    decode(std::string_view string) const = 0;
  };
}  // namespace libp2p::multi

#endif  // KAGOME_CODEC_HPP
