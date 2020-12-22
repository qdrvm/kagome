/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_INJECTOR_BLOCK_PRODUCING_NODE_INJECTOR_HPP
#define KAGOME_CORE_INJECTOR_BLOCK_PRODUCING_NODE_INJECTOR_HPP

#include "application/app_configuration.hpp"
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
      const application::AppConfiguration &app_config, Ts &&... args) {

    return di::make_injector(
        // inherit application injector
        makeApplicationInjector(app_config),
        // bind sr25519 keypair
        di::bind<crypto::Sr25519Keypair>.to(
            [](auto const &inj) { return get_sr25519_keypair(inj); }),
        // bind ed25519 keypair
        di::bind<crypto::Ed25519Keypair>.to(
            [](auto const &inj) { return get_ed25519_keypair(inj); }),
        // peer info
        di::bind<network::OwnPeerInfo>.to(
            [](const auto &injector) {
              return get_peer_info(injector);
            }),

        di::bind<consensus::Babe>.to(
            [](auto const &inj) { return get_babe(inj); }),
        di::bind<consensus::BabeLottery>.template to<consensus::BabeLotteryImpl>(),
        di::bind<network::BabeObserver>.to(
            [](auto const &inj) { return get_babe(inj); }),

        di::bind<consensus::grandpa::GrandpaObserver>.template to<consensus::grandpa::SyncingGrandpaObserver>()[di::override],
        di::bind<runtime::GrandpaApi>.template to<runtime::dummy::GrandpaApiDummy>()
            [boost::di::override],
        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }
}  // namespace kagome::injector

#endif  // KAGOME_CORE_INJECTOR_BLOCK_PRODUCING_NODE_INJECTOR_HPP
