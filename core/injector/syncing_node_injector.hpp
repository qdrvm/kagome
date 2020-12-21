/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_INJECTOR_SYNCING_NODE_INJECTOR_HPP
#define KAGOME_CORE_INJECTOR_SYNCING_NODE_INJECTOR_HPP

#include "application/app_configuration.hpp"
#include "consensus/babe/impl/syncing_babe.hpp"
#include "consensus/grandpa/impl/syncing_grandpa_observer.hpp"
#include "injector/application_injector.hpp"
#include "storage/in_memory/in_memory_storage.hpp"

namespace kagome::injector {
  namespace di = boost::di;

  template <typename Injector>
  auto get_peer_info(const Injector &injector) -> sptr<network::OwnPeerInfo> {
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

    const auto &config =
        injector.template create<const application::AppConfiguration &>();
    std::string multiaddress_str =
        "/ip4/0.0.0.0/tcp/" + std::to_string(config.p2pPort());
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
  }

  template <typename... Ts>
  auto makeSyncingNodeInjector(const application::AppConfiguration &app_config,
                               Ts &&... args) {
    using namespace boost;  // NOLINT;

    return di::make_injector(

        // inherit application injector
        makeApplicationInjector(app_config),

        // peer info
        di::bind<network::OwnPeerInfo>.to(
            [](const auto &injector) { return get_peer_info(injector); }),

        di::bind<consensus::Babe>.template to<consensus::SyncingBabe>()[di::override],
        di::bind<network::BabeObserver>.template to<consensus::SyncingBabe>()[di::override],
        di::bind<consensus::grandpa::GrandpaObserver>.template to<consensus::grandpa::SyncingGrandpaObserver>(),
        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }

}  // namespace kagome::injector

#endif  // KAGOME_CORE_INJECTOR_SYNCING_NODE_INJECTOR_HPP
