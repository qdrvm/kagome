/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_TRANSACTION_ANNOUNCE_HPP
#define KAGOME_NETWORK_TRANSACTION_ANNOUNCE_HPP

#include "primitives/extrinsic.hpp"

namespace kagome::network {
  /**
   * Announce a transaction on the network
   */
  struct TransactionAnnounce {
    primitives::Extrinsic extrinsic;
  };

  /**
   * @brief compares two BlockAnnounce instances
   * @param lhs first instance
   * @param rhs second instance
   * @return true if equal false otherwise
   */
  inline bool operator==(const TransactionAnnounce &lhs, const TransactionAnnounce &rhs) {
    return lhs.extrinsic == rhs.extrinsic;
  }
inline bool operator!=(const TransactionAnnounce &lhs, const TransactionAnnounce &rhs) {
    return !(lhs == rhs);
  }

  /**
   * @brief outputs object of type BlockAnnounce to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const TransactionAnnounce &v) {
    return s << v.extrinsic;
  }

  /**
   * @brief decodes object of type Block from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, TransactionAnnounce &v) {
    return s >> v.extrinsic;
  }
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_TRANSACTION_ANNOUNCE_HPP
