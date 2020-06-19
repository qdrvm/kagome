/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_INJECTOR_SYNCING_NODE_INJECTOR_HPP
#define KAGOME_CORE_INJECTOR_SYNCING_NODE_INJECTOR_HPP

#include "consensus/babe/impl/syncing_babe_observer.hpp"
#include "consensus/grandpa/impl/syncing_round_observer.hpp"
#include "injector/application_injector.hpp"
#include "storage/in_memory/in_memory_storage.hpp"

namespace kagome::injector {
  namespace di = boost::di;

  auto get_peer_info = [](const auto &injector,
                          uint16_t p2p_port) -> sptr<network::OwnPeerInfo> {
    static boost::optional<sptr<network::OwnPeerInfo>> initialized{boost::none};
    if (initialized) {
      return *initialized;
    }

    // get key storage
    auto &&local_pair = injector.template create<libp2p::crypto::KeyPair>();
    libp2p::crypto::PublicKey &public_key = local_pair.publicKey;
    auto &key_marshaller =
        injector.template create<libp2p::crypto::marshaller::KeyMarshaller &>();

    libp2p::peer::PeerId peer_id =
        libp2p::peer::PeerId::fromPublicKey(
            key_marshaller.marshal(public_key).value())
            .value();
    spdlog::debug("Received peer id: {}", peer_id.toBase58());
    std::string multiaddress_str =
        "/ip4/0.0.0.0/tcp/" + std::to_string(p2p_port);
    spdlog::debug("Received multiaddr: {}", multiaddress_str);
    auto multiaddress = libp2p::multi::Multiaddress::create(multiaddress_str);
    if (!multiaddress) {
      common::raise(multiaddress.error());  // exception
    }
    std::vector<libp2p::multi::Multiaddress> addresses;
    addresses.push_back(std::move(multiaddress.value()));

    initialized = std::make_shared<network::OwnPeerInfo>(std::move(peer_id),
                                                         std::move(addresses));
    return initialized.value();
  };

  template <typename... Ts>
  auto makeSyncingNodeInjector(
      const std::string &genesis_path,
      const std::string &leveldb_path,
      uint16_t p2p_port,
      const boost::asio::ip::tcp::endpoint &rpc_http_endpoint,
      const boost::asio::ip::tcp::endpoint &rpc_ws_endpoint,
      Ts &&... args) {
    using namespace boost;  // NOLINT;

    return di::make_injector(

        // inherit application injector
        makeApplicationInjector(
            genesis_path, leveldb_path, rpc_http_endpoint, rpc_ws_endpoint),

        // peer info
        di::bind<network::OwnPeerInfo>.to([p2p_port](const auto &injector) {
          return get_peer_info(injector, p2p_port);
        }),

        di::bind<network::BabeObserver>.template to<consensus::SyncingBabeObserver>(),
        di::bind<consensus::grandpa::RoundObserver>.template to<consensus::grandpa::SyncingRoundObserver>(),
        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }

}  // namespace kagome::injector

#endif  // KAGOME_CORE_INJECTOR_SYNCING_NODE_INJECTOR_HPP
