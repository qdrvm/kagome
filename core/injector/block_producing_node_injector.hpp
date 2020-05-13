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
#include "runtime/dummy/grandpa_dummy.hpp"
#include "storage/in_memory/in_memory_storage.hpp"

namespace kagome::injector {
  namespace di = boost::di;

  // sr25519 kp getter
  auto get_sr25519_keypair =
      [](const auto &injector) -> sptr<crypto::SR25519Keypair> {
    static auto initialized =
        boost::optional<sptr<crypto::SR25519Keypair>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    auto &key_storage = injector.template create<application::KeyStorage &>();
    auto &&sr25519_kp = key_storage.getLocalSr25519Keypair();

    initialized = std::make_shared<crypto::SR25519Keypair>(sr25519_kp);
    return initialized.value();
  };

  // ed25519 kp getter
  auto get_ed25519_keypair =
      [](const auto &injector) -> sptr<crypto::ED25519Keypair> {
    static auto initialized =
        boost::optional<sptr<crypto::ED25519Keypair>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    auto &key_storage = injector.template create<application::KeyStorage &>();
    auto &&ed25519_kp = key_storage.getLocalEd25519Keypair();

    initialized = std::make_shared<crypto::ED25519Keypair>(ed25519_kp);
    return initialized.value();
  };

  auto get_peer_keypair =
      [](const auto &injector) -> sptr<libp2p::crypto::KeyPair> {
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
  };

  auto get_peer_info = [](const auto &injector,
                          uint16_t p2p_port) -> sptr<libp2p::peer::PeerInfo> {
    static boost::optional<sptr<libp2p::peer::PeerInfo>> initialized{
        boost::none};
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
    libp2p::peer::PeerInfo peer_info{peer_id, std::move(addresses)};

    initialized =
        std::make_shared<libp2p::peer::PeerInfo>(std::move(peer_info));
    return initialized.value();
  };

  // key storage getter
  template <typename Injector>
  sptr<application::KeyStorage> get_key_storage(std::string_view keystore_path,
                                                const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<application::KeyStorage>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    auto &&result =
        application::LocalKeyStorage::create(std::string(keystore_path));
    if (!result) {
      common::raise(result.error());
    }
    initialized = result.value();
    return result.value();
  };

  auto get_babe = [](const auto &injector) -> sptr<consensus::Babe> {
    static auto initialized =
        boost::optional<sptr<consensus::BabeImpl>>(boost::none);
    if (initialized) {
      return *initialized;
    }

    initialized = std::make_shared<consensus::BabeImpl>(
        injector.template create<sptr<consensus::BabeLottery>>(),
        injector.template create<sptr<consensus::BlockExecutor>>(),
        injector.template create<sptr<storage::trie::TrieDb>>(),
        injector.template create<sptr<consensus::EpochStorage>>(),
        injector.template create<sptr<primitives::BabeConfiguration>>(),
        injector.template create<sptr<authorship::Proposer>>(),
        injector.template create<sptr<blockchain::BlockTree>>(),
        injector.template create<sptr<network::Gossiper>>(),
        injector.template create<crypto::SR25519Keypair>(),
        injector.template create<sptr<clock::SystemClock>>(),
        injector.template create<sptr<crypto::Hasher>>(),
        injector.template create<uptr<clock::Timer>>());
    return *initialized;
  };

  template <typename... Ts>
  auto makeBlockProducingNodeInjector(const std::string &genesis_path,
                                      const std::string &keystore_path,
                                      const std::string &leveldb_path,
                                      uint16_t p2p_port,
                                      uint16_t rpc_http_port,
                                      uint16_t rpc_ws_port,
                                      Ts &&... args) {
    using namespace boost;  // NOLINT;

    auto get_babe_observer = get_babe;

    return di::make_injector(

        // inherit application injector
        makeApplicationInjector(
            genesis_path, leveldb_path, rpc_http_port, rpc_ws_port),
        // bind sr25519 keypair
        di::bind<crypto::SR25519Keypair>.to(std::move(get_sr25519_keypair)),
        // bind ed25519 keypair
        di::bind<crypto::ED25519Keypair>.to(std::move(get_ed25519_keypair)),
        // compose peer keypair
        di::bind<libp2p::crypto::KeyPair>.to(
            std::move(get_peer_keypair))[boost::di::override],
        // peer info
        di::bind<libp2p::peer::PeerInfo>.to([p2p_port](const auto &injector) {
          return get_peer_info(injector, p2p_port);
        }),

        di::bind<consensus::Babe>.to(std::move(get_babe)),
        di::bind<consensus::BabeLottery>.template to<consensus::BabeLotteryImpl>(),
        di::bind<network::BabeObserver>.to(std::move(get_babe_observer)),

        di::bind<consensus::grandpa::RoundObserver>.template to<consensus::grandpa::SyncingRoundObserver>(),
        di::bind<storage::BufferStorage>.template to<storage::InMemoryStorage>()
            [boost::di::override],
        di::bind<application::KeyStorage>.to(
            [keystore_path](const auto &injector) {
              return get_key_storage(keystore_path, injector);
            }),
        di::bind<runtime::Grandpa>.to<runtime::dummy::GrandpaDummy>()
            [boost::di::override],
        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }
}  // namespace kagome::injector

#endif  // KAGOME_CORE_INJECTOR_BLOCK_PRODUCING_NODE_INJECTOR_HPP
