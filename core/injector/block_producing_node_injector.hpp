/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_INJECTOR_BLOCK_PRODUCING_NODE_INJECTOR_HPP
#define KAGOME_CORE_INJECTOR_BLOCK_PRODUCING_NODE_INJECTOR_HPP

#include "application/impl/local_key_storage.hpp"
#include "consensus/babe/impl/babe_impl.hpp"
#include "consensus/babe/impl/syncing_babe_observer.hpp"
#include "consensus/grandpa/impl/syncing_round_observer.hpp"
#include "injector/application_injector.hpp"
#include "injector/validating_node_injector.hpp"
#include "runtime/dummy/grandpa_dummy.hpp"
#include "storage/in_memory/in_memory_storage.hpp"

namespace kagome::injector {
  namespace di = boost::di;

  template <typename... Ts>
  auto makeBlockProducingNodeInjector(
      const std::string &genesis_path,
      const std::string &keystore_path,
      const std::string &leveldb_path,
      uint16_t p2p_port,
      const boost::asio::ip::tcp::endpoint &rpc_http_endpoint,
      const boost::asio::ip::tcp::endpoint &rpc_ws_endpoint,
      Ts &&... args) {
    using namespace boost;  // NOLINT;

    auto get_babe_observer = get_babe;

    return di::make_injector(

        // inherit application injector
        makeApplicationInjector(
            genesis_path, leveldb_path, rpc_http_endpoint, rpc_ws_endpoint),
        // bind sr25519 keypair
        di::bind<crypto::SR25519Keypair>.to(std::move(get_sr25519_keypair)),
        // bind ed25519 keypair
        di::bind<crypto::ED25519Keypair>.to(std::move(get_ed25519_keypair)),
        // compose peer keypair
        di::bind<libp2p::crypto::KeyPair>.to(
            std::move(get_peer_keypair))[boost::di::override],
        // peer info
        di::bind<network::OwnPeerInfo>.to([p2p_port](const auto &injector) {
          return get_peer_info(injector, p2p_port);
        }),

        di::bind<consensus::Babe>.to(std::move(get_babe)),
        di::bind<consensus::BabeLottery>.template to<consensus::BabeLotteryImpl>(),
        di::bind<network::BabeObserver>.to(std::move(get_babe_observer)),

        di::bind<consensus::grandpa::RoundObserver>.template to<consensus::grandpa::SyncingRoundObserver>(),
        di::bind<application::KeyStorage>.to(
            [keystore_path](const auto &injector) {
              return get_key_storage(keystore_path, injector);
            }),
        di::bind<runtime::Grandpa>.template to<runtime::dummy::GrandpaDummy>()
            [boost::di::override],
        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }
}  // namespace kagome::injector

#endif  // KAGOME_CORE_INJECTOR_BLOCK_PRODUCING_NODE_INJECTOR_HPP
