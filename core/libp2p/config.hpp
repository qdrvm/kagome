/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONFIG_HPP
#define KAGOME_CONFIG_HPP

#include <memory>
#include <vector>

#include "libp2p/crypto/key.hpp"
#include "libp2p/muxer/muxer_adaptor.hpp"
#include "libp2p/peer/peer_repository.hpp"
#include "libp2p/routing/routing_strategy.hpp"
#include "libp2p/transport/transport.hpp"

namespace libp2p {

  namespace detail {
    template <typename T>
    using vec = std::vector<T>;

    template <typename T>
    using sptr = std::shared_ptr<T>;

    template <typename T>
    using vecsptr = vec<sptr<T>>;
  }  // namespace detail

  struct Config {
    crypto::KeyPair peer_key;
    detail::vecsptr<transport::Transport> transports;
    detail::vecsptr<muxer::MuxerAdaptor> muxers;
    detail::vec<multi::Multiaddress> listen_addresses;
    detail::sptr<peer::PeerRepository> peer_repository;
    detail::sptr<routing::RoutingAdaptor> peer_routing;

    bool enable_ping = true;
  };

}  // namespace libp2p

#endif  // KAGOME_CONFIG_HPP
