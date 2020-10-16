/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PROPAGATED_TRANSACTIONS_HPP
#define KAGOME_NETWORK_PROPAGATED_TRANSACTIONS_HPP

#include "primitives/extrinsic.hpp"

namespace kagome::network {
  /**
   * Propagate transactions in network
   */
  struct PropagatedTransactions {
    std::vector<primitives::Extrinsic> extrinsics;
  };

  /**
   * @brief compares two PropagatedTransactions instances
   * @param lhs first instance
   * @param rhs second instance
   * @return true if equal false otherwise
   */
  inline bool operator==(const PropagatedTransactions &lhs,
                         const PropagatedTransactions &rhs) {
    return lhs.extrinsics == rhs.extrinsics;
  }
  inline bool operator!=(const PropagatedTransactions &lhs,
                         const PropagatedTransactions &rhs) {
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
  Stream &operator<<(Stream &s, const PropagatedTransactions &v) {
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
  Stream &operator>>(Stream &s, PropagatedTransactions &v) {
    return s >> v.extrinsics;
  }
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PROPAGATED_TRANSACTIONS_HPP
