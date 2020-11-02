/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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
    crypto::Sr25519Signature signature;
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
