/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_BOOTSTRAPNODES
#define KAGOME_NETWORK_BOOTSTRAPNODES

#include <libp2p/peer/peer_info.hpp>

namespace kagome::network {

  struct BootstrapNodes : public std::vector<libp2p::peer::PeerInfo> {
    using std::vector<libp2p::peer::PeerInfo>::vector;

    template <typename... Args>
    BootstrapNodes(Args... args)
        : std::vector<libp2p::peer::PeerInfo>::vector(args...) {}
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_BOOTSTRAPNODES
