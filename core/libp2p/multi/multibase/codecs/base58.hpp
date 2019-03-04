/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BASE58_HPP
#define KAGOME_BASE58_HPP

#include <optional>

#include "libp2p/multi/multibase/codec.hpp"

namespace libp2p::multi {
  /**
   * Encode/decode to/from base58 format
   * Implementation is taken from
   * https://github.com/bitcoin/bitcoin/blob/master/src/base58.h
   */
  class Base58Codec : public Codec {
   public:
    std::string encode(const kagome::common::Buffer &bytes) const override;

    kagome::expected::Result<kagome::common::Buffer, std::string> decode(
        std::string_view string) const override;

   private:
    /**
     * Actual implementation of the encoding
     * @param pbegin - pointer to the beginning of bytes collection
     * @param pend - pointer to the end of bytes collection
     * @return encoded string
     */
    std::string encodeImpl(const unsigned char *pbegin,
                           const unsigned char *pend) const;

    /**
     * Actual implementation of the decoding
     * @param psz - pointer to the string to be decoded
     * @return decoded bytes, if the process went successfully, none otherwise
     */
    std::optional<std::vector<unsigned char>> decodeImpl(const char *psz) const;
  };
};  // namespace libp2p::multi

#endif  // KAGOME_BASE58_HPP
