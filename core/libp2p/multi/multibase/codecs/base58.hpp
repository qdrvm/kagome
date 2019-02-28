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
    std::string encodeImpl(const unsigned char *pbegin,
                           const unsigned char *pend) const;

    std::optional<std::vector<unsigned char>> decodeImpl(const char *psz) const;
  };
};  // namespace libp2p::multi

#endif  // KAGOME_BASE58_HPP
