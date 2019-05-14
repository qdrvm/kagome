/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONFIG_HPP
#define KAGOME_CONFIG_HPP

#include <memory>
#include <vector>

#include "libp2p/crypto/key.hpp"
#include "libp2p/muxer/connection_muxer.hpp"
#include "libp2p/peer/peer_repository.hpp"
#include "libp2p/routing/router.hpp"
#include "libp2p/transport/transport.hpp"

namespace libp2p {

  template <typename T>
  using vec = std::vector<T>;

  template <typename T>
  using uptr = std::unique_ptr<T>;

  template <typename T>
  using vecuptr = vec<uptr<T>>;

  struct Config {
    crypto::KeyPair peer_key;
    vecuptr<transport::Transport> transports;
    vecuptr<muxer::StreamMuxer> muxers;
    vec<multi::Multiaddress> listen_addresses;
    uptr<peer::PeerRepository> peer_repository;
    uptr<routing::PeerRouting> peer_routing;

    bool enable_ping = true;
  };

}  // namespace libp2p

#endif  // KAGOME_CONFIG_HPP
