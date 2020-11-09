/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_INJECTOR_VALIDATING_NODE_INJECTOR_HPP
#define KAGOME_CORE_INJECTOR_VALIDATING_NODE_INJECTOR_HPP

#include "application/app_configuration.hpp"
#include "application/impl/local_key_storage.hpp"
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
    auto &key_storage = injector.template create<application::KeyStorage &>();
    auto &&sr25519_kp = key_storage.getLocalSr25519Keypair();

    initialized = std::make_shared<crypto::Sr25519Keypair>(sr25519_kp);
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
    auto &key_storage = injector.template create<application::KeyStorage &>();
    auto &&ed25519_kp = key_storage.getLocalEd25519Keypair();

    initialized = std::make_shared<crypto::Ed25519Keypair>(ed25519_kp);
    return initialized.value();
  }

  template <typename Injector>
  sptr<libp2p::crypto::KeyPair> get_peer_keypair(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<libp2p::crypto::KeyPair>>(boost::none);

    if (initialized) {
      return initialized.value();
    }

    // get key storage
    auto &keys = injector.template create<application::KeyStorage &>();
    auto &&local_pair = keys.getP2PKeypair();
    initialized = std::make_shared<libp2p::crypto::KeyPair>(local_pair);
    return initialized.value();
  }

  // peer info getter
  template <typename Injector>
  sptr<network::OwnPeerInfo> get_peer_info(const Injector &injector,
                                           uint16_t p2p_port) {
    static auto initialized =
        boost::optional<sptr<network::OwnPeerInfo>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    // get key storage
    auto &keys = injector.template create<application::KeyStorage &>();
    auto &&local_pair = keys.getP2PKeypair();
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
  }

  // key storage getter
  template <typename Injector>
  sptr<application::KeyStorage> get_key_storage(const application::AppConfiguration & config,
                                                const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<application::KeyStorage>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    auto chain_id = injector.template create<sptr<application::GenesisConfig>>()->id();
    auto keystore_path = config.keystore_path(chain_id);
    auto &&result =
        application::LocalKeyStorage::create(keystore_path);
    if (!result) {
      common::raise(result.error());
    }
    initialized = result.value();
    return result.value();
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
        injector.template create<sptr<authority::AuthorityUpdateObserver>>());
    return *initialized;
  }

  template <typename... Ts>
  auto makeFullNodeInjector(const application::AppConfiguration &app_config,
                            Ts &&... args) {
    using namespace boost;  // NOLINT;

    return di::make_injector(
        makeApplicationInjector(app_config),
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
        // compose peer info
        di::bind<network::OwnPeerInfo>.to(
            [p2p_port{app_config.p2p_port()}](const auto &injector) {
              return get_peer_info(injector, p2p_port);
            }),
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
            [is_only_finalizing{app_config.is_only_finalizing()}](
                const auto &injector) -> sptr<runtime::GrandpaApi> {
              static boost::optional<sptr<runtime::GrandpaApi>> initialized =
                  boost::none;
              if (initialized) {
                return *initialized;
              }
              if (is_only_finalizing) {
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
        di::bind<application::KeyStorage>.to(
            [&app_config](const auto &injector) {
              return get_key_storage(app_config, injector);
            }),
        di::bind<crypto::CryptoStore>.template to(
            [&app_config](const auto &injector) {
              return get_crypto_store(app_config, injector);
            })[boost::di::override],
        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }

}  // namespace kagome::injector

#endif  // KAGOME_CORE_INJECTOR_VALIDATING_NODE_INJECTOR_HPP
