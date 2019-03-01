/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multibase/codecs/base16.hpp"
#include "common/hexutil.hpp"

namespace libp2p::multi {
  using namespace kagome::common;
  using namespace kagome::expected;

  template <bool IsUpper>
  std::string Base16Codec<IsUpper>::encode(const Buffer &bytes) const {
    return hex<IsUpper>(bytes.toVector());
  }

  template <bool IsUpper>
  Result<Buffer, std::string> Base16Codec<IsUpper>::decode(
      std::string_view string) const {
    return map_error(unhex(string), [string](UnhexError err) {
      switch (err) {
        case UnhexError::kNotEnoughInput:
          return Error{"could not unhex string '" + std::string{string}
                           + "': input is either empty or contains odd number "
                             "of symbols"};
        case UnhexError::kNonHexInput:
          return Error{"could not unhex string '" + std::string{string}
                       + "': input is not hex-encoded"};
      }
    });
  }
}  // namespace libp2p::multi
