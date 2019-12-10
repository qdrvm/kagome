/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP
#define KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP

#include <boost/di.hpp>
#include <boost/di/extension/scopes/shared.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <outcome/outcome.hpp>

#include "api/extrinsic/extrinsic_api_service.hpp"
#include "api/extrinsic/impl/extrinsic_api_impl.hpp"
#include "api/transport/impl/http_session.hpp"
#include "api/transport/impl/listener_impl.hpp"
#include "application/impl/configuration_storage_impl.hpp"
#include "application/impl/local_key_storage.hpp"
#include "authorship/impl/block_builder_factory_impl.hpp"
#include "authorship/impl/block_builder_impl.hpp"
#include "authorship/impl/proposer_impl.hpp"
#include "blockchain/impl/block_tree_impl.hpp"
#include "blockchain/impl/key_value_block_header_repository.hpp"
#include "blockchain/impl/key_value_block_storage.hpp"
#include "boost/di/extension/injections/extensible_injector.hpp"
#include "clock/impl/basic_waitable_timer.hpp"
#include "clock/impl/clock_impl.hpp"
#include "common/outcome_throw.hpp"
#include "consensus/babe/babe_lottery.hpp"
#include "consensus/babe/common.hpp"
#include "consensus/babe/impl/babe_impl.hpp"
#include "consensus/babe/impl/babe_lottery_impl.hpp"
#include "consensus/babe/impl/babe_observer_impl.hpp"
#include "consensus/babe/impl/epoch_storage_dumb.hpp"
#include "consensus/grandpa/chain.hpp"
#include "consensus/grandpa/gossiper.hpp"
#include "consensus/grandpa/observer.hpp"
#include "consensus/grandpa/structs.hpp"
#include "consensus/grandpa/vote_graph.hpp"
#include "consensus/grandpa/vote_tracker.hpp"
#include "consensus/synchronizer/impl/synchronizer_impl.hpp"
#include "consensus/validation/babe_block_validator.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "crypto/vrf/vrf_provider_impl.hpp"
#include "extensions/extension_impl.hpp"
#include "network/impl/gossiper_broadcast.hpp"
#include "network/impl/router_libp2p.hpp"
#include "network/sync_protocol_client.hpp"
#include "network/sync_protocol_observer.hpp"
#include "runtime/impl/block_builder_api_impl.hpp"
#include "runtime/impl/core_impl.hpp"
#include "runtime/impl/grandpa_impl.hpp"
#include "runtime/impl/metadata_impl.hpp"
#include "runtime/impl/offchain_worker_impl.hpp"
#include "runtime/impl/parachain_host_impl.hpp"
#include "runtime/impl/storage_wasm_provider.hpp"
#include "runtime/impl/tagged_transaction_queue_impl.hpp"
#include "runtime/impl/wasm_memory_impl.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "storage/trie/impl/polkadot_codec.hpp"
#include "storage/trie/impl/polkadot_node.hpp"
#include "storage/trie/impl/polkadot_trie_db.hpp"
#include "transaction_pool/impl/pool_moderator_impl.hpp"
#include "transaction_pool/impl/transaction_pool_impl.hpp"

namespace kagome::injector {
  enum class InjectorError {
    INDEX_OUT_OF_BOUND = 1,  // index out of bound
  };
}

OUTCOME_HPP_DECLARE_ERROR(kagome::injector, InjectorError);

namespace kagome::injector {
  namespace di = boost::di;

  template <typename C>
  auto useConfig(C &&c) {
    return boost::di::bind<std::decay_t<C>>().template to(
        std::forward<C>(c))[boost::di::override];
  }

  template <class T>
  using sptr = std::shared_ptr<T>;

  template <class T>
  using uptr = std::unique_ptr<T>;

  template <class T>
  struct Wrapper {
    sptr<T> value;
  };

  template <typename... Ts>
  auto makeApplicationInjector(const std::string &genesis_path,
                               const std::string &keystore_path,
                               const std::string &leveldb_path,
                               Ts &&... args) {
    using namespace boost;  // NOLINT;

    // default values for configurations
    auto http_config = api::HttpSession::Configuration{};
    auto pool_moderator_config = transaction_pool::PoolModeratorImpl::Params{};
    auto synchronizer_config = consensus::SynchronizerConfig{};
    auto tp_pool_limits = transaction_pool::TransactionPool::Limits{
        transaction_pool::TransactionPoolImpl::kDefaultMaxReadyNum,
        transaction_pool::TransactionPoolImpl::kDefaultMaxWaitingNum};
    auto leveldb_options = leveldb::Options();

    auto get_sr25519_keypair =
        [](const auto &injector) -> sptr<crypto::SR25519Keypair> {
      auto &key_storage = injector.template create<application::KeyStorage &>();
      auto &&sr25519_kp = key_storage.getLocalSr25519Keypair();
      return std::make_shared<crypto::SR25519Keypair>(sr25519_kp);
    };

    auto get_ed25519_keypair =
        [](const auto &injector) -> sptr<crypto::ED25519Keypair> {
      auto &key_storage = injector.template create<application::KeyStorage &>();
      auto &&ed25519_kp = key_storage.getLocalEd25519Keypair();
      return std::make_shared<crypto::ED25519Keypair>(ed25519_kp);
    };

    auto get_peer_info =
        [](const auto &injector) -> sptr<libp2p::peer::PeerInfo> {
      // get key storage
      auto &keys = injector.template create<application::KeyStorage &>();
      auto &&local_pair = keys.getP2PKeypair();
      auto &public_key = local_pair.publicKey;
      libp2p::peer::PeerId peer_id =
          libp2p::peer::PeerId::fromPublicKey(public_key.data);
      // TODO (yuraz): specify port
      auto multiaddress =
          libp2p::multi::Multiaddress::create("/ip4/127.0.0.1/tcp/30363");
      if (!multiaddress) {
        common::raise(multiaddress.error());  // exception
      }
      std::vector<libp2p::multi::Multiaddress> addresses;
      addresses.push_back(std::move(multiaddress.value()));
      libp2p::peer::PeerInfo peer_info{peer_id, std::move(addresses)};

      return std::make_shared<libp2p::peer::PeerInfo>(std::move(peer_info));
    };

    auto get_authority_index =
        [](const auto &injector) -> sptr<primitives::AuthorityIndex> {
      auto core = injector.template create<sptr<runtime::Core>>();
      auto &keys = injector.template create<application::KeyStorage &>();
      auto &&local_pair = keys.getLocalEd25519Keypair();
      auto &public_key = local_pair.public_key;
      auto &&authorities_res = core->authorities();
      if (authorities_res.has_error()) {
        common::raise(authorities_res.error());
      }
      auto &&authorities = authorities_res.value();

      uint64_t index = 0;
      for (auto &auth : authorities) {
        if (public_key == auth.id) {
          break;
        }
        ++index;
      }
      if (index >= authorities.size()) {
        common::raise(InjectorError::INDEX_OUT_OF_BOUND);
      }
      return std::make_shared<primitives::AuthorityIndex>(
          primitives::AuthorityIndex{index});
    };

    auto get_extrinsic_api_listener =
        [](const auto &injector) -> sptr<api::Listener> {
      // listener is used currently only for extrinsic api
      // if other apis are required, need to
      // implement lambda creating corresponding api service
      // where listener is initialized manually
      // in this case listener should be bound

      auto &context = injector.template create<boost::asio::io_context &>();
      auto &cfg =
          injector.template create<application::ConfigurationStorage &>();
      auto extrinsic_tcp_version = boost::asio::ip::tcp::v4();
      uint16_t extrinsic_api_port = cfg.getExtrinsicApiPort();
      api::ListenerImpl::Configuration listener_config{
          boost::asio::ip::tcp::endpoint{extrinsic_tcp_version,
                                         extrinsic_api_port}};
      auto &&http_session_config =
          injector.template create<api::HttpSession::Configuration>();

      return std::make_shared<api::ListenerImpl>(
          context, listener_config, http_session_config);
    };

    // make injector
    return di::make_injector(
        // inherit host injector
        libp2p::injector::makeHostInjector(),
        // bind sr25519 keypair
        di::bind<crypto::SR25519Keypair>.template to(std::move(get_sr25519_keypair)),
        // bind ed25519 keypair
        di::bind<crypto::ED25519Keypair>.template to(std::move(get_ed25519_keypair)),
        // compose peer info
        di::bind<libp2p::peer::PeerInfo>.template to(std::move(get_peer_info)),
        // find and bind authority id
        di::bind<primitives::AuthorityIndex>.template to(std::move(get_authority_index)),

        // bind io_context: 1 per injector
        di::bind<::boost::asio::io_context>.in(
            di::extension::shared)[boost::di::override],

        // bind configs
        injector::useConfig(http_config),
        injector::useConfig(pool_moderator_config),
        injector::useConfig(synchronizer_config),
        injector::useConfig(tp_pool_limits),
        injector::useConfig(leveldb_options),

        // bind interfaces
        di::bind<api::Listener>.to(std::move(get_extrinsic_api_listener)),
        di::bind<api::ExtrinsicApi>.template to<api::ExtrinsicApiImpl>(),
        di::bind<authorship::Proposer>.template to<authorship::ProposerImpl>(),
        di::bind<authorship::BlockBuilder>.template to<authorship::BlockBuilderImpl>(),
        di::bind<authorship::BlockBuilderFactory>.template to<authorship::BlockBuilderFactoryImpl>(),
        di::bind<Wrapper<storage::PersistentBufferMap>>.template to(
            [obj = std::make_shared<Wrapper<storage::PersistentBufferMap>>()]
            (const auto &injector) mutable ->std::shared_ptr<Wrapper<storage::PersistentBufferMap>> {
          return obj;
        }),
        di::bind<storage::PersistentBufferMap>.template to(
            [&](const auto &injector) -> sptr<storage::PersistentBufferMap> {
              auto &wrapper = injector.template create<Wrapper<storage::PersistentBufferMap>&>();
              if (nullptr == wrapper.value) {
                auto options = leveldb::Options{};
                options.create_if_missing = true;
                auto db = storage::LevelDB::create(leveldb_path, options);
                if (!db) {
                  common::raise(db.error());
                }
                wrapper.value = sptr<storage::PersistentBufferMap>(
                    dynamic_cast<storage::PersistentBufferMap *>(
                        db.value().release()));
              }

              return wrapper.value;
            }),
        // create block storage
        di::bind<blockchain::BlockStorage>.to(
            [&](const auto &injector)
                -> std::shared_ptr<blockchain::BlockStorage> {
              auto &&hasher =
                  injector.template create<std::shared_ptr<crypto::Hasher>>();

              const auto &db = injector.template create<
                  std::shared_ptr<storage::trie::TrieDb>>();

              auto storage =
                  blockchain::KeyValueBlockStorage::createWithGenesis(db,
                                                                      hasher);
              if (storage.has_error()) {
                common::raise(storage.error());
              }
              return storage.value();
            }),
        // create block tree
        di::bind<blockchain::BlockTree>.to(
            [&](const auto &injector)
                -> std::shared_ptr<blockchain::BlockTree> {
              auto header_repo = injector.template create<
                  std::shared_ptr<blockchain::BlockHeaderRepository>>();

              auto &&storage = injector.template create<
                  std::shared_ptr<blockchain::BlockStorage>>();

              // block id is zero for first time
              const primitives::BlockId block_id = common::Hash256{};

              auto &&hasher =
                  injector.template create<std::shared_ptr<crypto::Hasher>>();

              auto &&tree = blockchain::BlockTreeImpl::create(
                  std::move(header_repo), storage, block_id, std::move(hasher));
              if (!tree) {
                common::raise(tree.error());
              }

              return tree.value();
            }),
        di::bind<blockchain::BlockHeaderRepository>.template to<blockchain::KeyValueBlockHeaderRepository>(),
        di::bind<clock::SystemClock>.template to<clock::SystemClockImpl>(),
        di::bind<clock::SteadyClock>.template to<clock::SteadyClockImpl>(),
        di::bind<clock::Timer>.template to<clock::BasicWaitableTimer>().in(
            di::unique),
        di::bind<consensus::Babe>.template to<consensus::BabeImpl>(),
        di::bind<consensus::BabeLottery>.template to<consensus::BabeLotteryImpl>(),
        di::bind<network::BabeObserver>.template to<consensus::BabeObserverImpl>(),
        di::bind<consensus::EpochStorage>.template to<consensus::EpochStorageDumb>(),
        di::bind<consensus::Synchronizer>.template to<consensus::SynchronizerImpl>(),
        di::bind<consensus::BlockValidator>.template to<consensus::BabeBlockValidator>(),
        di::bind<crypto::ED25519Provider>.template to<crypto::ED25519ProviderImpl>(),
        di::bind<crypto::Hasher>.template to<crypto::HasherImpl>(),
        di::bind<crypto::SR25519Provider>.template to<crypto::SR25519ProviderImpl>(),
        di::bind<crypto::VRFProvider>.template to<crypto::VRFProviderImpl>(),
        di::bind<extensions::Extension>.template to<extensions::ExtensionImpl>(),
        di::bind<network::BabeGossiper>.template to<network::GossiperBroadcast>(),
        di::bind<consensus::grandpa::Gossiper>.template to<network::GossiperBroadcast>(),
        di::bind<network::Gossiper>.template to<network::GossiperBroadcast>(),
        di::bind<network::Router>.template to<network::RouterLibp2p>(),
        di::bind<network::SyncProtocolClient>.template to<consensus::SynchronizerImpl>(),
        di::bind<network::SyncProtocolObserver>.template to<consensus::SynchronizerImpl>(),
        di::bind<runtime::WasmMemory>.template to<runtime::WasmMemoryImpl>(),
        di::bind<runtime::TaggedTransactionQueue>.template to<runtime::TaggedTransactionQueueImpl>(),
        di::bind<runtime::ParachainHost>.template to<runtime::ParachainHostImpl>(),
        di::bind<runtime::OffchainWorker>.template to<runtime::OffchainWorkerImpl>(),
        di::bind<runtime::Metadata>.template to<runtime::MetadataImpl>(),
        di::bind<runtime::Grandpa>.template to<runtime::GrandpaImpl>(),
        di::bind<runtime::Core>.template to<runtime::CoreImpl>(),
        di::bind<runtime::BlockBuilderApi>.template to<runtime::BlockBuilderApiImpl>(),
        di::bind<transaction_pool::TransactionPool>.template to<transaction_pool::TransactionPoolImpl>(),
        di::bind<transaction_pool::PoolModerator>.template to<transaction_pool::PoolModeratorImpl>(),
        di::bind<storage::trie::TrieDb>.template to([&](const auto& injector){
          auto configuration_storage = injector.template create<
              std::shared_ptr<application::ConfigurationStorage>>();
          const auto &genesis_raw_configs =
              configuration_storage->getGenesis();

          auto persistent_storage = injector.template create<sptr<storage::PersistentBufferMap>>();
          auto trie_db = std::make_shared<storage::trie::PolkadotTrieDb>(persistent_storage);
          for (const auto &[key, val] : genesis_raw_configs) {
            if (auto res = trie_db->put(key, val); not res) {
              common::raise(res.error());
            }
          }
          return trie_db;
        }),
        di::bind<storage::trie::Codec>.template to<storage::trie::PolkadotCodec>(),
        di::bind<runtime::WasmProvider>.template to<runtime::StorageWasmProvider>().in(
            di::extension::shared),
        // create configuration storage shared value
        di::bind<application::ConfigurationStorage>.to(
            [&](const auto &injector)
                -> std::shared_ptr<application::ConfigurationStorage> {
              auto config_storage_res =
                  application::ConfigurationStorageImpl::create(genesis_path);
              if (config_storage_res.has_error()) {
                common::raise(config_storage_res.error());
              }
              return config_storage_res.value();
            }),
        // create key storage shared value
        di::bind<application::KeyStorage>.to(
            [&](const auto &injector)
                -> std::shared_ptr<application::KeyStorage> {
              auto &&result =
                  application::LocalKeyStorage::create(keystore_path);
              if (!result) {
                common::raise(result.error());
              }
              return result.value();
            }),

        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }
}  // namespace kagome::injector

#endif  // KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP
