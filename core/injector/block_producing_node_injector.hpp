/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_INJECTOR_BLOCK_PRODUCING_NODE_INJECTOR_HPP
#define KAGOME_CORE_INJECTOR_BLOCK_PRODUCING_NODE_INJECTOR_HPP

#include "application/app_config.hpp"
#include "application/impl/local_key_storage.hpp"
#include "consensus/babe/impl/babe_impl.hpp"
#include "consensus/grandpa/impl/syncing_grandpa_observer.hpp"
#include "injector/application_injector.hpp"
#include "injector/validating_node_injector.hpp"
#include "runtime/dummy/grandpa_api_dummy.hpp"
#include "storage/in_memory/in_memory_storage.hpp"

namespace kagome::injector {
  namespace di = boost::di;

  template <typename... Ts>
  auto makeBlockProducingNodeInjector(
      const application::AppConfigPtr &app_config, Ts &&... args) {
    using namespace boost;  // NOLINT;
    assert(app_config);

    return di::make_injector(

        // inherit application injector
        makeApplicationInjector(app_config->genesis_path(),
                                app_config->leveldb_path(),
                                app_config->rpc_http_endpoint(),
                                app_config->rpc_ws_endpoint()),
        // bind sr25519 keypair
        di::bind<crypto::Sr25519Keypair>.to(
            [](auto const &inj) { return get_sr25519_keypair(inj); }),
        // bind ed25519 keypair
        di::bind<crypto::Ed25519Keypair>.to(
            [](auto const &inj) { return get_ed25519_keypair(inj); }),
        // compose peer keypair
        di::bind<libp2p::crypto::KeyPair>.to([](auto const &inj) {
          return get_peer_keypair(inj);
        })[boost::di::override],
        // peer info
        di::bind<network::OwnPeerInfo>.to(
            [p2p_port{app_config->p2p_port()}](const auto &injector) {
              return get_peer_info(injector, p2p_port);
            }),

        di::bind<consensus::Babe>.to(
            [](auto const &inj) { return get_babe(inj); }),
        di::bind<consensus::BabeLottery>.template to<consensus::BabeLotteryImpl>(),
        di::bind<network::BabeObserver>.to(
            [](auto const &inj) { return get_babe(inj); }),

        di::bind<consensus::grandpa::GrandpaObserver>.template to<consensus::grandpa::SyncingGrandpaObserver>(),
        di::bind<application::KeyStorage>.to(
            [app_config](const auto &injector) {
              return get_key_storage(app_config->keystore_path(), injector);
            }),
        di::bind<runtime::GrandpaApi>.template to<runtime::dummy::GrandpaApiDummy>()
            [boost::di::override],
        di::bind<crypto::CryptoStore>.template to(
            [app_config](const auto &injector) {
              return get_crypto_store(app_config->keystore_path(), injector);
            })[boost::di::override],
        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }
}  // namespace kagome::injector

#endif  // KAGOME_CORE_INJECTOR_BLOCK_PRODUCING_NODE_INJECTOR_HPP
