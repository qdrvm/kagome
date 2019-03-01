/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BASE16_HPP
#define KAGOME_BASE16_HPP

#include "libp2p/multi/multibase/codec.hpp"

namespace libp2p::multi {
  /**
   * Encode/decode to/from base16 format
   * @tparam IsUpper - true, if this codec is to work with upper-case hex
   * strings, false, if with lower-case
   */
  template <bool IsUpper>
  class Base16Codec : public Codec {
   public:
    std::string encode(const kagome::common::Buffer &bytes) const override;

    kagome::expected::Result<kagome::common::Buffer, std::string> decode(
        std::string_view string) const override;
  };
}  // namespace libp2p::multi

#endif  // KAGOME_BASE16_HPP
