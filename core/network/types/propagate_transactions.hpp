/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PROPAGATED_TRANSACTIONS_HPP
#define KAGOME_NETWORK_PROPAGATED_TRANSACTIONS_HPP

#include "primitives/transaction.hpp"

namespace kagome::network {
  /**
   * Propagate transactions in network
   */
  struct PropagatedExtrinsics {
    std::vector<primitives::Extrinsic> extrinsics;
  };

  /**
   * @brief compares two PropagatedTransactions instances
   * @param lhs first instance
   * @param rhs second instance
   * @return true if equal false otherwise
   */
  inline bool operator==(const PropagatedExtrinsics &lhs,
                         const PropagatedExtrinsics &rhs) {
    return lhs.extrinsics == rhs.extrinsics;
  }
  inline bool operator!=(const PropagatedExtrinsics &lhs,
                         const PropagatedExtrinsics &rhs) {
    return !(lhs == rhs);
  }

  /**
   * @brief outputs object of type PropagatedTransactions to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const PropagatedExtrinsics &v) {
    return s << v.extrinsics;
  }

  /**
   * @brief decodes object of type PropagatedTransactions from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, PropagatedExtrinsics &v) {
    return s >> v.extrinsics;
  }
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PROPAGATED_TRANSACTIONS_HPP
