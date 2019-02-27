/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTIBASE_CODEC_HPP
#define KAGOME_MULTIBASE_CODEC_HPP

#include <memory>
#include <string_view>

#include "libp2p/multi/multibase.hpp"

namespace libp2p::multi {
  /**
   * Encodes and decodes data, given a base
   */
  class Codec {
   public:
    explicit Codec(Multibase::Encoding base);

    /**
     * Encode the data
     * @param input to be encoded
     * @param include_encoding
     * @return data in encoded format
     */
    std::string encode(std::string_view input, bool include_encoding = true);

    /**
     * Decode the data
     * @param input to be decoded
     * @return decoded data
     */
    std::string decode(std::string_view input);

    /**
     * Get base of thise codes
     * @return the base
     */
    Multibase::Encoding base() const;

   private:
    class Impl;
    std::unique_ptr<Impl> impl_;
  };

}  // namespace libp2p::multi

#endif  // KAGOME_MULTIBASE_CODEC_HPP
