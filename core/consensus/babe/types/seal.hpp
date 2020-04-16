/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_SEAL_HPP
#define KAGOME_SEAL_HPP

#include "crypto/sr25519_types.hpp"

namespace kagome::consensus {
  /**
   * Basically a signature of the block's header
   */
  struct Seal {
    /// Sig_sr25519(Blake2s(block_header))
    crypto::SR25519Signature signature;
  };

  /**
   * @brief outputs object of type Seal to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const Seal &seal) {
    return s << seal.signature;
  }

  /**
   * @brief decodes object of type Seal from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, Seal &seal) {
    return s >> seal.signature;
  }
}  // namespace kagome::consensus

#endif  // KAGOME_SEAL_HPP
