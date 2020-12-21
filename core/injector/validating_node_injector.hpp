/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_INJECTOR_VALIDATING_NODE_INJECTOR_HPP
#define KAGOME_CORE_INJECTOR_VALIDATING_NODE_INJECTOR_HPP

#include "application/app_configuration.hpp"
#include "consensus/babe/impl/babe_impl.hpp"
#include "consensus/grandpa/chain.hpp"
#include "consensus/grandpa/impl/grandpa_impl.hpp"
#include "injector/application_injector.hpp"
#include "network/types/own_peer_info.hpp"
#include "runtime/binaryen/runtime_api/grandpa_api_impl.hpp"

namespace kagome::injector {

  // sr25519 kp getter
  template <typename Injector>
  sptr<crypto::Sr25519Keypair> get_sr25519_keypair(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<crypto::Sr25519Keypair>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    const crypto::CryptoStore &crypto_store =
        injector.template create<const crypto::CryptoStore &>();
    auto &&sr25519_kp = crypto_store.getBabeKeypair();
    if (not sr25519_kp) {
      spdlog::error("Failed to get BABE keypair");
      return nullptr;
    }

    initialized = std::make_shared<crypto::Sr25519Keypair>(sr25519_kp.value());
    return initialized.value();
  }

  // ed25519 kp getter
  template <typename Injector>
  sptr<crypto::Ed25519Keypair> get_ed25519_keypair(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<crypto::Ed25519Keypair>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    auto const &crypto_store =
        injector.template create<const crypto::CryptoStore &>();
    auto &&ed25519_kp = crypto_store.getGrandpaKeypair();
    if (not ed25519_kp) {
      spdlog::error("Failed to get GRANDPA keypair");
      return nullptr;
    }

    initialized = std::make_shared<crypto::Ed25519Keypair>(ed25519_kp.value());
    return initialized.value();
  }

  // peer info getter
  template <typename Injector>
  sptr<network::OwnPeerInfo> get_peer_info(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<network::OwnPeerInfo>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    // get key storage
    auto &&local_pair = get_peer_keypair(injector);
    libp2p::crypto::PublicKey &public_key = local_pair->publicKey;
    auto &key_marshaller =
        injector.template create<libp2p::crypto::marshaller::KeyMarshaller &>();

    libp2p::peer::PeerId peer_id =
        libp2p::peer::PeerId::fromPublicKey(
            key_marshaller.marshal(public_key).value())
            .value();
    spdlog::debug("Received peer id: {}", peer_id.toBase58());
    application::AppConfiguration const &config =
        injector.template create<application::AppConfiguration const &>();
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

  template <typename Injector>
  sptr<consensus::Babe> get_babe(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<consensus::BabeImpl>>(boost::none);
    if (initialized) {
      return *initialized;
    }

    initialized = std::make_shared<consensus::BabeImpl>(
        injector.template create<sptr<application::AppStateManager>>(),
        injector.template create<sptr<consensus::BabeLottery>>(),
        injector.template create<sptr<consensus::BlockExecutor>>(),
        injector.template create<sptr<storage::trie::TrieStorage>>(),
        injector.template create<sptr<consensus::EpochStorage>>(),
        injector.template create<sptr<primitives::BabeConfiguration>>(),
        injector.template create<sptr<authorship::Proposer>>(),
        injector.template create<sptr<blockchain::BlockTree>>(),
        injector.template create<sptr<network::Gossiper>>(),
        injector.template create<sptr<crypto::Sr25519Provider>>(),
        injector.template create<crypto::Sr25519Keypair>(),
        injector.template create<sptr<clock::SystemClock>>(),
        injector.template create<sptr<crypto::Hasher>>(),
        injector.template create<uptr<clock::Timer>>(),
        injector.template create<sptr<authority::AuthorityUpdateObserver>>(),
        injector.template create<consensus::SlotsStrategy>());
    return *initialized;
  }

  template <typename... Ts>
  auto makeValidatingNodeInjector(
      const application::AppConfiguration &app_config, Ts &&... args) {
    using namespace boost;  // NOLINT;

    return di::make_injector(
        makeApplicationInjector(app_config),
        // bind sr25519 keypair
        di::bind<crypto::Sr25519Keypair>.to(
            [](auto const &inj) { return get_sr25519_keypair(inj); }),
        // bind ed25519 keypair
        di::bind<crypto::Ed25519Keypair>.to(
            [](auto const &inj) { return get_ed25519_keypair(inj); }),
        // compose peer info
        di::bind<network::OwnPeerInfo>.to(
            [](const auto &injector) { return get_peer_info(injector); }),
        di::bind<consensus::Babe>.to(
            [](auto const &inj) { return get_babe(inj); }),
        di::bind<consensus::BabeLottery>.template to<consensus::BabeLotteryImpl>(),
        di::bind<network::BabeObserver>.to(
            [](auto const &inj) { return get_babe(inj); }),
        di::bind<consensus::grandpa::RoundObserver>.template to<consensus::grandpa::GrandpaImpl>(),
        di::bind<consensus::grandpa::CatchUpObserver>.template to<consensus::grandpa::GrandpaImpl>(),
        di::bind<consensus::grandpa::GrandpaObserver>.template to<consensus::grandpa::GrandpaImpl>(),
        di::bind<consensus::grandpa::Grandpa>.template to<consensus::grandpa::GrandpaImpl>(),
        di::bind<runtime::GrandpaApi>.template to(
            [](const auto &injector) -> sptr<runtime::GrandpaApi> {
              static boost::optional<sptr<runtime::GrandpaApi>> initialized =
                  boost::none;
              if (initialized) {
                return *initialized;
              }
              application::AppConfiguration const &config =
                  injector
                      .template create<application::AppConfiguration const &>();
              if (config.isOnlyFinalizing()) {
                auto grandpa_api = injector.template create<
                    sptr<runtime::binaryen::GrandpaApiImpl>>();
                initialized = grandpa_api;
              } else {
                auto grandpa_api = injector.template create<
                    sptr<runtime::binaryen::GrandpaApiImpl>>();
                initialized = grandpa_api;
              }
              return *initialized;
            })[di::override],
        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }

}  // namespace kagome::injector

#endif  // KAGOME_CORE_INJECTOR_VALIDATING_NODE_INJECTOR_HPP
