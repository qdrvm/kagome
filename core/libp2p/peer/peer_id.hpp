/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_ID_HPP
#define KAGOME_PEER_ID_HPP

#include "libp2p/multi/multihash.hpp"

namespace libp2p::peer {

  /**
   * Any valid multihash can represent valid PeerId
   */
  using PeerId = multi::Multihash;

}  // namespace libp2p::peer

#endif  // KAGOME_PEER_ID_HPP
