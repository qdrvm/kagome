/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KAGOME_CONFIG_HPP
#define KAGOME_KAGOME_CONFIG_HPP

#include <libp2p/peer/peer_info.hpp>
#include "primitives/block.hpp"
#include "crypto/ed25519_types.hpp"
#include "crypto/sr25519_types.hpp"

namespace kagome::application {

  /**
   * Contains configuration parameters of a chain, such as genesis blocks or
   * authority keys
   */
  struct KagomeConfig {
    primitives::Block genesis;
    std::vector<libp2p::peer::PeerId> peer_ids;
    std::vector<crypto::SR25519PublicKey> session_keys;
    std::vector<crypto::ED25519PublicKey> authorities;
  };

};  // namespace kagome::application

#endif  // KAGOME_KAGOME_CONFIG_HPP
