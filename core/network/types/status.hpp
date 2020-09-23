/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_TYPES_STATUS_HPP
#define KAGOME_CORE_NETWORK_TYPES_STATUS_HPP

#include <vector>
#include <algorithm>
#include <libp2p/peer/peer_info.hpp>

#include "network/types/roles.hpp"
#include "primitives/common.hpp"

using kagome::primitives::BlockNumber;
using kagome::primitives::BlockHash;

namespace kagome::network {

  /**
   * Is the structure to send to a new connected peer. It contains common
   * information about current peer and used by the remote peer to detect the
   * posibility of the correct communication with it.
   */
  struct Status {
    /**
     * Protocol version.
     */
    uint32_t version;

    /**
     * Minimum supported version.
     */
    uint32_t min_supported_version;

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

    /**
     * Chain-specific status.
     */
    std::vector<uint8_t> chain_status;
  };

  /**
   * @brief compares two Status instances
   * @param lhs first instance
   * @param rhs second instance
   * @return true if equal false otherwise
   */
  inline bool operator==(const Status &lhs, const Status &rhs) {
    return lhs.version == rhs.version
           && lhs.min_supported_version == rhs.min_supported_version
           && lhs.roles == rhs.roles && lhs.best_number == rhs.best_number
           && lhs.best_hash == rhs.best_hash
           && lhs.genesis_hash == rhs.genesis_hash
           && std::equal(lhs.chain_status.begin(),
                         lhs.chain_status.end(),
                         rhs.chain_status.begin());
  }

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
    return s << v.version << v.min_supported_version << v.roles << v.best_number
             << v.best_hash << v.genesis_hash << v.chain_status;
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
    return s >> v.version >> v.min_supported_version >> v.roles >> v.best_number
           >> v.best_hash >> v.genesis_hash >> v.chain_status;
  }

}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_TYPES_STATUS_HPP