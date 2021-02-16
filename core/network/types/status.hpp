/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_TYPES_STATUS_HPP
#define KAGOME_CORE_NETWORK_TYPES_STATUS_HPP

#include <algorithm>
#include <libp2p/peer/peer_info.hpp>
#include <vector>

#include "network/types/roles.hpp"
#include "primitives/common.hpp"

namespace kagome::network {

  using kagome::primitives::BlockHash;
  using kagome::primitives::BlockNumber;

  /**
   * Is the structure to send to a new connected peer. It contains common
   * information about current peer and used by the remote peer to detect the
   * posibility of the correct communication with it.
   */
  struct Status {
    /**
     * Supported roles.
     */
    Roles roles;

    /**
     * Best block number.
     */
    BlockNumber best_number;

    /**
     * Best block hash.
     */
    BlockHash best_hash;

    /**
     * Genesis block hash.
     */
    BlockHash genesis_hash;
  };

  /**
   * @brief outputs object of type Status to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const Status &v) {
    return s << v.roles << v.best_number << v.best_hash << v.genesis_hash;
  }

  /**
   * @brief decodes object of type Status from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, Status &v) {
    return s >> v.roles >> v.best_number >> v.best_hash >> v.genesis_hash;
  }

}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_TYPES_STATUS_HPP
