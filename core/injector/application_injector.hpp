/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP
#define KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP

#include <boost/di.hpp>
#include <boost/di/extension/scopes/shared.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/injector/kademlia_injector.hpp>

#include "api/service/author/author_jrpc_processor.hpp"
#include "api/service/author/impl/author_api_impl.hpp"
#include "api/service/chain/chain_jrpc_processor.hpp"
#include "api/service/chain/impl/chain_api_impl.hpp"
#include "api/service/impl/api_service_impl.hpp"
#include "api/service/payment/impl/payment_api_impl.hpp"
#include "api/service/payment/payment_jrpc_processor.hpp"
#include "api/service/rpc/impl/rpc_api_impl.hpp"
#include "api/service/rpc/rpc_jrpc_processor.hpp"
#include "api/service/state/impl/state_api_impl.hpp"
#include "api/service/state/state_jrpc_processor.hpp"
#include "api/service/system/impl/system_api_impl.hpp"
#include "api/service/system/system_jrpc_processor.hpp"
#include "api/transport/impl/http/http_listener_impl.hpp"
#include "api/transport/impl/http/http_session.hpp"
#include "api/transport/impl/ws/ws_listener_impl.hpp"
#include "api/transport/impl/ws/ws_session.hpp"
#include "api/transport/rpc_thread_pool.hpp"
#include "application/app_configuration.hpp"
#include "application/impl/app_state_manager_impl.hpp"
#include "application/impl/chain_spec_impl.hpp"
#include "authorship/impl/block_builder_factory_impl.hpp"
#include "authorship/impl/block_builder_impl.hpp"
#include "authorship/impl/proposer_impl.hpp"
#include "blockchain/impl/block_tree_impl.hpp"
#include "blockchain/impl/key_value_block_header_repository.hpp"
#include "blockchain/impl/key_value_block_storage.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "boost/di/extension/injections/extensible_injector.hpp"
#include "clock/impl/basic_waitable_timer.hpp"
#include "clock/impl/clock_impl.hpp"
#include "common/outcome_throw.hpp"
#include "consensus/authority/authority_manager.hpp"
#include "consensus/authority/authority_update_observer.hpp"
#include "consensus/authority/impl/authority_manager_impl.hpp"
#include "consensus/babe/babe_lottery.hpp"
#include "consensus/babe/common.hpp"
#include "consensus/babe/impl/babe_lottery_impl.hpp"
#include "consensus/babe/impl/babe_synchronizer_impl.hpp"
#include "consensus/babe/impl/babe_util_impl.hpp"
#include "consensus/babe/impl/block_executor.hpp"
#include "consensus/babe/types/slots_strategy.hpp"
#include "consensus/grandpa/finalization_observer.hpp"
#include "consensus/grandpa/impl/environment_impl.hpp"
#include "consensus/grandpa/impl/finalization_composite.hpp"
#include "consensus/grandpa/impl/grandpa_impl.hpp"
#include "consensus/grandpa/impl/vote_crypto_provider_impl.hpp"
#include "consensus/grandpa/structs.hpp"
#include "consensus/grandpa/vote_graph.hpp"
#include "consensus/grandpa/vote_tracker.hpp"
#include "consensus/validation/babe_block_validator.hpp"
#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/crypto_store/crypto_store_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "crypto/vrf/vrf_provider_impl.hpp"
#include "host_api/impl/host_api_factory_impl.hpp"
#include "network/impl/dummy_sync_protocol_client.hpp"
#include "network/impl/extrinsic_observer_impl.hpp"
#include "network/impl/gossiper_broadcast.hpp"
#include "network/impl/kademlia_storage_backend.hpp"
#include "network/impl/peer_manager_impl.hpp"
#include "network/impl/remote_sync_protocol_client.hpp"
#include "network/impl/router_libp2p.hpp"
#include "network/impl/sync_protocol_observer_impl.hpp"
#include "network/sync_protocol_client.hpp"
#include "network/sync_protocol_observer.hpp"
#include "network/types/sync_clients_set.hpp"
#include "outcome/outcome.hpp"
#include "runtime/binaryen/module/wasm_module_factory_impl.hpp"
#include "runtime/binaryen/module/wasm_module_impl.hpp"
#include "runtime/binaryen/runtime_api/account_nonce_api_impl.hpp"
#include "runtime/binaryen/runtime_api/babe_api_impl.hpp"
#include "runtime/binaryen/runtime_api/block_builder_impl.hpp"
#include "runtime/binaryen/runtime_api/core_factory_impl.hpp"
#include "runtime/binaryen/runtime_api/core_impl.hpp"
#include "runtime/binaryen/runtime_api/grandpa_api_impl.hpp"
#include "runtime/binaryen/runtime_api/metadata_impl.hpp"
#include "runtime/binaryen/runtime_api/offchain_worker_impl.hpp"
#include "runtime/binaryen/runtime_api/parachain_host_impl.hpp"
#include "runtime/binaryen/runtime_api/tagged_transaction_queue_impl.hpp"
#include "runtime/binaryen/runtime_api/transaction_payment_api_impl.hpp"
#include "runtime/common/storage_wasm_provider.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/impl/trie_storage_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_node.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "transaction_pool/impl/pool_moderator_impl.hpp"
#include "transaction_pool/impl/transaction_pool_impl.hpp"

namespace kagome::injector {
  namespace di = boost::di;
  namespace fs = boost::filesystem;

  template <typename C>
  auto useConfig(C c) {
    return boost::di::bind<std::decay_t<C>>().template to(
        std::move(c))[boost::di::override];
  }

  template <class T>
  using sptr = std::shared_ptr<T>;

  template <class T>
  using uptr = std::unique_ptr<T>;

  template <typename Injector>
  sptr<api::ApiServiceImpl> get_jrpc_api_service(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<api::ApiServiceImpl>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    auto storage_sub_engine = injector.template create<
        primitives::events::StorageSubscriptionEnginePtr>();

    auto chain_sub_engine =
        injector
            .template create<primitives::events::ChainSubscriptionEnginePtr>();

    auto ext_sub_engine = injector.template create<
        primitives::events::ExtrinsicSubscriptionEnginePtr>();

    auto ext_event_key_repo = injector.template create<
        std::shared_ptr<subscription::ExtrinsicEventKeyRepository>>();

    auto app_state_manager =
        injector
            .template create<std::shared_ptr<application::AppStateManager>>();
    auto rpc_thread_pool =
        injector.template create<std::shared_ptr<api::RpcThreadPool>>();
    std::vector<std::shared_ptr<api::Listener>> listeners{
        injector.template create<std::shared_ptr<api::HttpListenerImpl>>(),
        injector.template create<std::shared_ptr<api::WsListenerImpl>>(),
    };
    auto server = injector.template create<std::shared_ptr<api::JRpcServer>>();
    std::vector<std::shared_ptr<api::JRpcProcessor>> processors{
        injector
            .template create<std::shared_ptr<api::state::StateJrpcProcessor>>(),
        injector.template create<
            std::shared_ptr<api::author::AuthorJRpcProcessor>>(),
        injector
            .template create<std::shared_ptr<api::chain::ChainJrpcProcessor>>(),
        injector.template create<
            std::shared_ptr<api::system::SystemJrpcProcessor>>(),
        injector.template create<std::shared_ptr<api::rpc::RpcJRpcProcessor>>(),
        injector.template create<
            std::shared_ptr<api::payment::PaymentJRpcProcessor>>()};
    auto block_tree = injector.template create<sptr<blockchain::BlockTree>>();
    const auto &trie_storage =
        injector.template create<sptr<storage::trie::TrieStorage>>();

    initialized =
        std::make_shared<api::ApiServiceImpl>(std::move(app_state_manager),
                                              std::move(rpc_thread_pool),
                                              std::move(listeners),
                                              std::move(server),
                                              processors,
                                              std::move(storage_sub_engine),
                                              std::move(chain_sub_engine),
                                              std::move(ext_sub_engine),
                                              std::move(ext_event_key_repo),
                                              std::move(block_tree),
                                              std::move(trie_storage));

    auto state_api = injector.template create<std::shared_ptr<api::StateApi>>();
    state_api->setApiService(initialized.value());

    auto chain_api = injector.template create<std::shared_ptr<api::ChainApi>>();
    chain_api->setApiService(initialized.value());

    auto author_api =
        injector.template create<std::shared_ptr<api::AuthorApi>>();
    author_api->setApiService(initialized.value());

    return initialized.value();
  }

  // jrpc api listener (over HTTP) getter
  template <typename Injector>
  sptr<api::HttpListenerImpl> get_jrpc_api_http_listener(
      const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<api::HttpListenerImpl>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    const application::AppConfiguration &config =
        injector.template create<application::AppConfiguration const &>();
    auto &endpoint = config.rpcHttpEndpoint();

    auto app_state_manager =
        injector.template create<sptr<application::AppStateManager>>();

    auto context = injector.template create<sptr<api::RpcContext>>();

    api::HttpListenerImpl::Configuration listener_config;
    listener_config.endpoint = endpoint;

    auto &&http_session_config =
        injector.template create<api::HttpSession::Configuration>();

    initialized = std::make_shared<api::HttpListenerImpl>(
        app_state_manager, context, listener_config, http_session_config);
    return initialized.value();
  }

  // jrpc api listener (over Websockets) getter
  template <typename Injector>
  sptr<api::WsListenerImpl> get_jrpc_api_ws_listener(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<api::WsListenerImpl>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    const application::AppConfiguration &config =
        injector.template create<application::AppConfiguration const &>();
    auto &endpoint = config.rpcWsEndpoint();

    auto app_state_manager =
        injector.template create<sptr<application::AppStateManager>>();

    auto context = injector.template create<sptr<api::RpcContext>>();

    api::WsListenerImpl::Configuration listener_config;
    listener_config.endpoint = endpoint;

    auto &&ws_session_config =
        injector.template create<api::WsSession::Configuration>();

    initialized = std::make_shared<api::WsListenerImpl>(
        app_state_manager, context, listener_config, ws_session_config);
    return initialized.value();
  }

  // block storage getter
  template <typename Injector>
  sptr<blockchain::BlockStorage> get_block_storage(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<blockchain::BlockStorage>>(boost::none);

    if (initialized) {
      return initialized.value();
    }
    auto &&hasher = injector.template create<sptr<crypto::Hasher>>();

    const auto &db = injector.template create<sptr<storage::BufferStorage>>();

    const auto &trie_storage =
        injector.template create<sptr<storage::trie::TrieStorage>>();

    auto storage = blockchain::KeyValueBlockStorage::create(
        trie_storage->getRootHash(),
        db,
        hasher,
        [&db, &injector](const primitives::Block &genesis_block) {
          // handle genesis initialization, which happens when there is not
          // authorities and last completed round in the storage
          if (not db->get(storage::kAuthoritySetKey)) {
            // insert authorities
            auto grandpa_api =
                injector.template create<sptr<runtime::GrandpaApi>>();
            const auto &weighted_authorities_res = grandpa_api->authorities(
                primitives::BlockId(primitives::BlockNumber{0}));
            BOOST_ASSERT_MSG(weighted_authorities_res,
                             "grandpa_api_->authorities failed");
            const auto &weighted_authorities = weighted_authorities_res.value();

            auto log = log::createLogger("injector", "kagome");

            for (const auto authority : weighted_authorities) {
              log->info("Grandpa authority: {}", authority.id.id.toHex());
            }

            consensus::grandpa::VoterSet voters{0};
            for (const auto &weighted_authority : weighted_authorities) {
              voters.insert(
                  primitives::GrandpaSessionKey{weighted_authority.id.id},
                  weighted_authority.weight);
              log->debug("Added to grandpa authorities: {}, weight: {}",
                         weighted_authority.id.id.toHex(),
                         weighted_authority.weight);
            }
            BOOST_ASSERT_MSG(voters.size() != 0, "Grandpa voters are empty");
            auto authorities_put_res =
                db->put(storage::kAuthoritySetKey,
                        common::Buffer(scale::encode(voters).value()));
            if (not authorities_put_res) {
              BOOST_ASSERT_MSG(false, "Could not insert authorities");
              std::exit(1);
            }
          }
        });
    if (storage.has_error()) {
      common::raise(storage.error());
    }
    initialized = storage.value();
    return initialized.value();
  }

  // block tree getter
  template <typename Injector>
  sptr<blockchain::BlockTree> get_block_tree(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<blockchain::BlockTree>>(boost::none);

    if (initialized) {
      return initialized.value();
    }
    auto header_repo =
        injector.template create<sptr<blockchain::BlockHeaderRepository>>();

    auto storage = injector.template create<sptr<blockchain::BlockStorage>>();

    auto last_finalized_block_res = storage->getLastFinalizedBlockHash();

    const auto block_id =
        last_finalized_block_res.has_value()
            ? primitives::BlockId{last_finalized_block_res.value()}
            : primitives::BlockId{0};

    auto extrinsic_observer =
        injector.template create<sptr<network::ExtrinsicObserver>>();

    auto hasher = injector.template create<sptr<crypto::Hasher>>();

    auto chain_events_engine =
        injector
            .template create<primitives::events::ChainSubscriptionEnginePtr>();
    auto ext_events_engine = injector.template create<
        primitives::events::ExtrinsicSubscriptionEnginePtr>();
    auto ext_events_key_repo = injector.template create<
        std::shared_ptr<subscription::ExtrinsicEventKeyRepository>>();

    auto runtime_core =
        injector.template create<std::shared_ptr<runtime::Core>>();
    auto babe_configuration =
        injector
            .template create<std::shared_ptr<primitives::BabeConfiguration>>();
    auto babe_util =
        injector.template create<std::shared_ptr<consensus::BabeUtil>>();

    auto tree =
        blockchain::BlockTreeImpl::create(std::move(header_repo),
                                          storage,
                                          block_id,
                                          std::move(extrinsic_observer),
                                          std::move(hasher),
                                          std::move(chain_events_engine),
                                          std::move(ext_events_engine),
                                          std::move(ext_events_key_repo),
                                          std::move(runtime_core),
                                          std::move(babe_configuration),
                                          std::move(babe_util));
    if (!tree) {
      common::raise(tree.error());
    }
    initialized = tree.value();
    return initialized.value();
  }

  template <typename Injector>
  sptr<host_api::HostApiFactoryImpl> get_host_api_factory(
      const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<host_api::HostApiFactoryImpl>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    auto tracker =
        injector.template create<sptr<storage::changes_trie::ChangesTracker>>();
    auto sr25519_provider =
        injector.template create<sptr<crypto::Sr25519Provider>>();
    auto ed25519_provider =
        injector.template create<sptr<crypto::Ed25519Provider>>();
    auto secp256k1_provider =
        injector.template create<sptr<crypto::Secp256k1Provider>>();
    auto hasher = injector.template create<sptr<crypto::Hasher>>();
    auto crypto_store = injector.template create<sptr<crypto::CryptoStore>>();
    auto bip39_provider =
        injector.template create<sptr<crypto::Bip39Provider>>();

    initialized =
        std::make_shared<host_api::HostApiFactoryImpl>(tracker,
                                                       sr25519_provider,
                                                       ed25519_provider,
                                                       secp256k1_provider,
                                                       hasher,
                                                       crypto_store,
                                                       bip39_provider);
    return initialized.value();
  }

  template <typename Injector>
  sptr<storage::trie::TrieStorageBackendImpl> get_trie_storage_backend(
      const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<storage::trie::TrieStorageBackendImpl>>(
            boost::none);

    if (initialized) {
      return initialized.value();
    }
    auto storage = injector.template create<sptr<storage::BufferStorage>>();
    using blockchain::prefix::TRIE_NODE;
    auto backend = std::make_shared<storage::trie::TrieStorageBackendImpl>(
        storage, common::Buffer{TRIE_NODE});
    initialized = backend;
    return backend;
  }

  template <typename Injector>
  sptr<storage::trie::TrieStorageImpl> get_trie_storage_impl(
      const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<storage::trie::TrieStorageImpl>>(boost::none);

    if (initialized) {
      return initialized.value();
    }
    auto factory =
        injector.template create<sptr<storage::trie::PolkadotTrieFactory>>();
    auto codec = injector.template create<sptr<storage::trie::Codec>>();
    auto serializer =
        injector.template create<sptr<storage::trie::TrieSerializer>>();
    auto tracker =
        injector.template create<sptr<storage::changes_trie::ChangesTracker>>();
    auto trie_storage_res = storage::trie::TrieStorageImpl::createEmpty(
        factory, codec, serializer, tracker);
    if (!trie_storage_res) {
      common::raise(trie_storage_res.error());
    }

    sptr<storage::trie::TrieStorageImpl> trie_storage =
        std::move(trie_storage_res.value());
    initialized = trie_storage;
    return trie_storage;
  }

  template <typename Injector>
  sptr<storage::trie::TrieStorage> get_trie_storage(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<storage::trie::TrieStorage>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    auto configuration_storage =
        injector.template create<sptr<application::ChainSpec>>();
    const auto &genesis_raw_configs = configuration_storage->getGenesis();

    auto trie_storage =
        injector.template create<sptr<storage::trie::TrieStorageImpl>>();
    auto batch = trie_storage->getPersistentBatch();
    if (not batch) {
      common::raise(batch.error());
    }

    auto log = log::createLogger("injector", "kagome");

    for (const auto &[key, val] : genesis_raw_configs) {
      log->debug("Key: {}, Val: {}", key.toHex(), val.toHex().substr(0, 200));
      if (auto res = batch.value()->put(key, val); not res) {
        common::raise(res.error());
      }
    }
    if (auto res = batch.value()->commit(); not res) {
      common::raise(res.error());
    }

    initialized = trie_storage;
    return trie_storage;
  }

  // level db getter
  template <typename Injector>
  sptr<storage::BufferStorage> get_level_db(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<storage::BufferStorage>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    const application::AppConfiguration &config =
        injector.template create<application::AppConfiguration const &>();

    auto genesis_config =
        injector.template create<sptr<application::ChainSpec>>();
    auto options = leveldb::Options{};
    options.create_if_missing = true;
    auto db = storage::LevelDB::create(
        config.databasePath(genesis_config->id()), options);
    if (!db) {
      auto log = log::createLogger("injector", "kagome");
      log->critical("Can't create LevelDB in {}: {}",
                    fs::absolute(config.databasePath(genesis_config->id()),
                                 fs::current_path())
                        .native(),
                    db.error().message());
      exit(EXIT_FAILURE);
    }
    initialized = db.value();
    return initialized.value();
  }

  // configuration storage getter
  template <typename Injector>
  std::shared_ptr<application::ChainSpec> get_genesis_config(
      const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<application::ChainSpec>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    const application::AppConfiguration &config =
        injector.template create<application::AppConfiguration const &>();
    auto const &genesis_path = config.genesisPath();

    auto genesis_config_res =
        application::ChainSpecImpl::loadFrom(genesis_path.native());
    if (genesis_config_res.has_error()) {
      common::raise(genesis_config_res.error());
    }
    initialized = genesis_config_res.value();
    return genesis_config_res.value();
  }

  template <typename Injector>
  sptr<primitives::BabeConfiguration> get_babe_configuration(
      const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<primitives::BabeConfiguration>>(boost::none);
    if (initialized) {
      return *initialized;
    }

    auto babe_api = injector.template create<sptr<runtime::BabeApi>>();
    auto configuration_res = babe_api->configuration();
    if (not configuration_res) {
      common::raise(configuration_res.error());
    }
    auto config = configuration_res.value();
    auto log = log::createLogger("injector", "kagome");
    for (const auto &authority : config.genesis_authorities) {
      log->debug("Babe authority: {}", authority.id.id.toHex());
    }
    config.leadership_rate.first *= 3;
    initialized = std::make_shared<primitives::BabeConfiguration>(config);
    return *initialized;
  }

  template <typename Injector>
  consensus::SlotsStrategy get_slots_strategy(const Injector &injector) {
    static auto initialized =
        boost::optional<consensus::SlotsStrategy>(boost::none);
    if (not initialized) {
      const application::AppConfiguration &config =
          injector.template create<const application::AppConfiguration &>();
      initialized = config.isUnixSlotsStrategy()
                        ? consensus::SlotsStrategy::FromUnixEpoch
                        : consensus::SlotsStrategy::FromZero;
    }
    return *initialized;
  }

  template <class Injector>
  sptr<crypto::KeyFileStorage> get_key_file_storage(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<crypto::KeyFileStorage>>(boost::none);
    if (initialized) {
      return *initialized;
    }

    const application::AppConfiguration &config =
        injector.template create<application::AppConfiguration const &>();
    auto genesis_config =
        injector.template create<sptr<application::ChainSpec>>();

    auto path = config.keystorePath(genesis_config->id());
    auto key_file_storage_res = crypto::KeyFileStorage::createAt(path);
    if (not key_file_storage_res) {
      common::raise(key_file_storage_res.error());
    }
    initialized = std::move(key_file_storage_res.value());

    return *initialized;
  }

  template <class Injector>
  sptr<consensus::grandpa::FinalizationObserver> get_finalization_observer(
      const Injector &injector) {
    static auto instance = boost::optional<
        std::shared_ptr<consensus::grandpa::FinalizationObserver>>(boost::none);
    if (instance) {
      return *instance;
    }

    instance = std::make_shared<consensus::grandpa::FinalizationComposite>(
        injector.template create<
            std::shared_ptr<authority::AuthorityManagerImpl>>());

    return *instance;
  }

  template <class Injector>
  sptr<network::Router> get_router(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<network::Router>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    initialized = std::make_shared<network::RouterLibp2p>(
        injector.template create<sptr<application::AppStateManager>>(),
        injector.template create<libp2p::Host &>(),
        injector.template create<const application::AppConfiguration &>(),
        injector.template create<sptr<application::ChainSpec>>(),
        injector.template create<network::OwnPeerInfo &>(),
        injector.template create<sptr<network::StreamEngine>>(),
        injector.template create<sptr<network::BabeObserver>>(),
        injector.template create<sptr<consensus::grandpa::GrandpaObserver>>(),
        injector.template create<sptr<network::SyncProtocolObserver>>(),
        injector.template create<sptr<network::ExtrinsicObserver>>(),
        injector.template create<sptr<network::Gossiper>>(),
        injector.template create<const network::BootstrapNodes &>(),
        injector.template create<sptr<blockchain::BlockStorage>>(),
        injector.template create<sptr<libp2p::protocol::Ping>>(),
        injector.template create<sptr<network::PeerManager>>(),
        injector.template create<sptr<blockchain::BlockTree>>(),
        injector.template create<sptr<crypto::Hasher>>());
    return initialized.value();
  }

  template <class Injector>
  sptr<network::PeerManager> get_peer_manager(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<network::PeerManager>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    initialized = std::make_shared<network::PeerManagerImpl>(
        injector.template create<sptr<application::AppStateManager>>(),
        injector.template create<libp2p::Host &>(),
        injector.template create<sptr<libp2p::protocol::Identify>>(),
        injector.template create<sptr<libp2p::protocol::kademlia::Kademlia>>(),
        injector.template create<sptr<libp2p::protocol::Scheduler>>(),
        injector.template create<sptr<network::StreamEngine>>(),
        injector.template create<const application::AppConfiguration &>(),
        injector.template create<const application::ChainSpec &>(),
        injector.template create<sptr<clock::SteadyClock>>(),
        injector.template create<const network::BootstrapNodes &>(),
        injector.template create<const network::OwnPeerInfo &>(),
        injector.template create<sptr<network::SyncClientsSet>>(),
        injector.template create<sptr<blockchain::BlockTree>>(),
        injector.template create<sptr<crypto::Hasher>>(),
        injector.template create<sptr<blockchain::BlockStorage>>(),
        injector.template create<sptr<network::BabeObserver>>());
    return initialized.value();
  }

  template <typename Injector>
  sptr<libp2p::crypto::KeyPair> get_peer_keypair(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<libp2p::crypto::KeyPair>>(boost::none);

    if (initialized) {
      return initialized.value();
    }

    auto &app_config =
        injector.template create<const application::AppConfiguration &>();
    auto &crypto_provider =
        injector.template create<const crypto::Ed25519Provider &>();
    auto &crypto_store =
        injector.template create<const crypto::CryptoStore &>();

    auto log = log::createLogger("injector", "kagome");

    if (app_config.nodeKey()) {
      log->info("Will use LibP2P keypair from config or args");

      auto provided_keypair =
          crypto_provider.generateKeypair(app_config.nodeKey().value());
      BOOST_ASSERT(provided_keypair.secret_key == app_config.nodeKey().value());

      auto &&pub = provided_keypair.public_key;
      auto &&priv = provided_keypair.secret_key;

      initialized =
          std::make_shared<libp2p::crypto::KeyPair>(libp2p::crypto::KeyPair{
              .publicKey = {{.type = libp2p::crypto::Key::Type::Ed25519,
                             .data = {pub.begin(), pub.end()}}},
              .privateKey = {{.type = libp2p::crypto::Key::Type::Ed25519,
                              .data = {priv.begin(), priv.end()}}}});
      return initialized.value();
    }

    if (crypto_store.getLibp2pKeypair()) {
      log->info("Will use LibP2P keypair from key storage");

      auto stored_keypair = crypto_store.getLibp2pKeypair().value();

      initialized =
          std::make_shared<libp2p::crypto::KeyPair>(std::move(stored_keypair));
      return initialized.value();
    }

    log->warn(
        "Can not get LibP2P keypair from crypto storage. "
        "Unique one will be generated for current running");

    auto generated_keypair = crypto_provider.generateKeypair();

    auto &&pub = generated_keypair.public_key;
    auto &&priv = generated_keypair.secret_key;

    initialized =
        std::make_shared<libp2p::crypto::KeyPair>(libp2p::crypto::KeyPair{
            .publicKey = {{.type = libp2p::crypto::Key::Type::Ed25519,
                           .data = {pub.begin(), pub.end()}}},
            .privateKey = {{.type = libp2p::crypto::Key::Type::Ed25519,
                            .data = {priv.begin(), priv.end()}}}});

    return initialized.value();
  }

  template <typename Injector>
  sptr<consensus::BlockExecutor> get_block_executor(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<consensus::BlockExecutor>>(boost::none);
    if (initialized) {
      return *initialized;
    }

    initialized = std::make_shared<consensus::BlockExecutor>(
        injector.template create<sptr<blockchain::BlockTree>>(),
        injector.template create<sptr<runtime::Core>>(),
        injector.template create<sptr<primitives::BabeConfiguration>>(),
        injector.template create<sptr<consensus::BabeSynchronizer>>(),
        injector.template create<sptr<consensus::BlockValidator>>(),
        injector.template create<sptr<consensus::grandpa::Environment>>(),
        injector.template create<sptr<transaction_pool::TransactionPool>>(),
        injector.template create<sptr<crypto::Hasher>>(),
        injector.template create<sptr<authority::AuthorityUpdateObserver>>(),
        injector.template create<sptr<consensus::BabeUtil>>(),
        injector.template create<sptr<boost::asio::io_context>>(),
        injector.template create<uptr<clock::Timer>>());
    return *initialized;
  }

  template <typename Injector>
  sptr<libp2p::protocol::kademlia::Config> get_kademlia_config(
      const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<libp2p::protocol::kademlia::Config>>(boost::none);
    if (initialized) {
      return *initialized;
    }

    auto &chain_spec = injector.template create<application::ChainSpec &>();

    auto kagome_config = std::make_shared<libp2p::protocol::kademlia::Config>(
        libp2p::protocol::kademlia::Config{
            .protocolId = "/" + chain_spec.protocolId() + "/kad",
            .maxBucketSize = 1000,
            .randomWalk = {.interval = std::chrono::minutes(1)}});

    initialized = std::move(kagome_config);
    return *initialized;
  }

  template <typename... Ts>
  auto makeApplicationInjector(const application::AppConfiguration &config,
                               Ts &&... args) {
    using namespace boost;  // NOLINT;

    // default values for configurations
    api::RpcThreadPool::Configuration rpc_thread_pool_config{};
    api::HttpSession::Configuration http_config{};
    api::WsSession::Configuration ws_config{};
    transaction_pool::PoolModeratorImpl::Params pool_moderator_config{};
    transaction_pool::TransactionPool::Limits tp_pool_limits{};
    libp2p::protocol::PingConfig ping_config{};

    return di::make_injector(
        // bind configs
        injector::useConfig(rpc_thread_pool_config),
        injector::useConfig(http_config),
        injector::useConfig(ws_config),
        injector::useConfig(pool_moderator_config),
        injector::useConfig(tp_pool_limits),
        injector::useConfig(ping_config),

        // inherit host injector
        libp2p::injector::makeHostInjector(
            libp2p::injector::useSecurityAdaptors<
                libp2p::security::Noise>()[di::override]),

        // inherit kademlia injector
        libp2p::injector::makeKademliaInjector(),
        di::bind<libp2p::protocol::kademlia::Config>.to([](auto const &inj) {
          return get_kademlia_config(inj);
        })[boost::di::override],

        di::bind<application::AppStateManager>.template to<application::AppStateManagerImpl>(),
        di::bind<application::AppConfiguration>.to(config),

        // compose peer keypair
        di::bind<libp2p::crypto::KeyPair>.to([](auto const &inj) {
          return get_peer_keypair(inj);
        })[boost::di::override],

        // bind io_context: 1 per injector
        di::bind<::boost::asio::io_context>.in(
            di::extension::shared)[boost::di::override],

        // bind interfaces
        di::bind<api::HttpListenerImpl>.to([](const auto &injector) {
          return get_jrpc_api_http_listener(injector);
        }),
        di::bind<api::WsListenerImpl>.to([](const auto &injector) {
          return get_jrpc_api_ws_listener(injector);
        }),
        di::bind<libp2p::crypto::random::RandomGenerator>.template to<libp2p::crypto::random::BoostRandomGenerator>()
            [di::override],
        di::bind<api::AuthorApi>.template to<api::AuthorApiImpl>(),
        di::bind<api::ChainApi>.template to<api::ChainApiImpl>(),
        di::bind<api::StateApi>.template to<api::StateApiImpl>(),
        di::bind<api::SystemApi>.template to<api::SystemApiImpl>(),
        di::bind<api::RpcApi>.template to<api::RpcApiImpl>(),
        di::bind<api::PaymentApi>.template to<api::PaymentApiImpl>(),
        di::bind<api::ApiService>.to([](const auto &injector) {
          return get_jrpc_api_service(injector);
        }),
        di::bind<api::JRpcServer>.template to<api::JRpcServerImpl>(),
        di::bind<authorship::Proposer>.template to<authorship::ProposerImpl>(),
        di::bind<authorship::BlockBuilder>.template to<authorship::BlockBuilderImpl>(),
        di::bind<authorship::BlockBuilderFactory>.template to<authorship::BlockBuilderFactoryImpl>(),
        di::bind<storage::BufferStorage>.to(
            [](const auto &injector) { return get_level_db(injector); }),
        di::bind<blockchain::BlockStorage>.to(
            [](const auto &injector) { return get_block_storage(injector); }),
        di::bind<blockchain::BlockTree>.to(
            [](auto const &inj) { return get_block_tree(inj); }),
        di::bind<blockchain::BlockHeaderRepository>.template to<blockchain::KeyValueBlockHeaderRepository>(),
        di::bind<clock::SystemClock>.template to<clock::SystemClockImpl>(),
        di::bind<clock::SteadyClock>.template to<clock::SteadyClockImpl>(),
        di::bind<clock::Timer>.template to<clock::BasicWaitableTimer>(),
        di::bind<primitives::BabeConfiguration>.to([](auto const &injector) {
          return get_babe_configuration(injector);
        }),
        di::bind<consensus::BabeSynchronizer>.template to<consensus::BabeSynchronizerImpl>(),
        di::bind<consensus::SlotsStrategy>.template to(
            [](const auto &injector) { return get_slots_strategy(injector); }),
        di::bind<consensus::grandpa::Environment>.template to<consensus::grandpa::EnvironmentImpl>(),
        di::bind<consensus::BlockValidator>.template to<consensus::BabeBlockValidator>(),
        di::bind<crypto::Ed25519Provider>.template to<crypto::Ed25519ProviderImpl>(),
        di::bind<crypto::Hasher>.template to<crypto::HasherImpl>(),
        di::bind<crypto::Sr25519Provider>.template to<crypto::Sr25519ProviderImpl>(),
        di::bind<crypto::VRFProvider>.template to<crypto::VRFProviderImpl>(),
        di::bind<network::StreamEngine>.template to<network::StreamEngine>(),
        di::bind<crypto::Bip39Provider>.template to<crypto::Bip39ProviderImpl>(),
        di::bind<crypto::Pbkdf2Provider>.template to<crypto::Pbkdf2ProviderImpl>(),
        di::bind<crypto::Secp256k1Provider>.template to<crypto::Secp256k1ProviderImpl>(),
        di::bind<crypto::KeyFileStorage>.template to([](auto const &injector) {
          return get_key_file_storage(injector);
        }),
        di::bind<crypto::CryptoStore>.template to<crypto::CryptoStoreImpl>(),
        di::bind<host_api::HostApiFactory>.template to(
            [](auto const &injector) {
              return get_host_api_factory(injector);
            }),
        di::bind<consensus::BabeGossiper>.template to<network::GossiperBroadcast>(),
        di::bind<consensus::grandpa::Gossiper>.template to<network::GossiperBroadcast>(),
        di::bind<network::Gossiper>.template to<network::GossiperBroadcast>(),
        di::bind<network::SyncProtocolObserver>.template to<network::SyncProtocolObserverImpl>(),
        di::bind<runtime::binaryen::WasmModule>.template to<runtime::binaryen::WasmModuleImpl>(),
        di::bind<runtime::binaryen::WasmModuleFactory>.template to<runtime::binaryen::WasmModuleFactoryImpl>(),
        di::bind<runtime::binaryen::CoreFactory>.template to<runtime::binaryen::CoreFactoryImpl>(),
        di::bind<runtime::binaryen::RuntimeEnvironmentFactory>.template to<runtime::binaryen::RuntimeEnvironmentFactoryImpl>(),
        di::bind<runtime::TaggedTransactionQueue>.template to<runtime::binaryen::TaggedTransactionQueueImpl>(),
        di::bind<runtime::ParachainHost>.template to<runtime::binaryen::ParachainHostImpl>(),
        di::bind<runtime::OffchainWorker>.template to<runtime::binaryen::OffchainWorkerImpl>(),
        di::bind<runtime::Metadata>.template to<runtime::binaryen::MetadataImpl>(),
        di::bind<runtime::GrandpaApi>.template to<runtime::binaryen::GrandpaApiImpl>(),
        di::bind<runtime::Core>.template to<runtime::binaryen::CoreImpl>(),
        di::bind<runtime::BabeApi>.template to<runtime::binaryen::BabeApiImpl>(),
        di::bind<runtime::BlockBuilder>.template to<runtime::binaryen::BlockBuilderImpl>(),
        di::bind<runtime::TransactionPaymentApi>.template to<runtime::binaryen::TransactionPaymentApiImpl>(),
        di::bind<runtime::AccountNonceApi>.template to<runtime::binaryen::AccountNonceApiImpl>(),
        di::bind<runtime::TrieStorageProvider>.template to<runtime::TrieStorageProviderImpl>(),
        di::bind<transaction_pool::TransactionPool>.template to<transaction_pool::TransactionPoolImpl>(),
        di::bind<transaction_pool::PoolModerator>.template to<transaction_pool::PoolModeratorImpl>(),
        di::bind<storage::changes_trie::ChangesTracker>.template to<storage::changes_trie::StorageChangesTrackerImpl>(),
        di::bind<storage::trie::TrieStorageBackend>.to(
            [](auto const &inj) { return get_trie_storage_backend(inj); }),
        di::bind<storage::trie::TrieStorageImpl>.to(
            [](auto const &inj) { return get_trie_storage_impl(inj); }),
        di::bind<storage::trie::TrieStorage>.to(
            [](auto const &inj) { return get_trie_storage(inj); }),
        di::bind<storage::trie::PolkadotTrieFactory>.template to<storage::trie::PolkadotTrieFactoryImpl>(),
        di::bind<storage::trie::Codec>.template to<storage::trie::PolkadotCodec>(),
        di::bind<storage::trie::TrieSerializer>.template to<storage::trie::TrieSerializerImpl>(),
        di::bind<runtime::WasmProvider>.template to<runtime::StorageWasmProvider>(),
        di::bind<application::ChainSpec>.to(
            [](const auto &injector) { return get_genesis_config(injector); }),
        di::bind<network::ExtrinsicObserver>.template to<network::ExtrinsicObserverImpl>(),
        di::bind<network::ExtrinsicGossiper>.template to<network::GossiperBroadcast>(),
        di::bind<authority::AuthorityUpdateObserver>.template to<authority::AuthorityManagerImpl>(),
        di::bind<authority::AuthorityManager>.template to<authority::AuthorityManagerImpl>(),
        di::bind<consensus::grandpa::FinalizationObserver>.to(
            [](auto const &inj) { return get_finalization_observer(inj); }),
        di::bind<network::PeerManager>.to(
            [](auto const &inj) { return get_peer_manager(inj); }),
        di::bind<network::Router>.to(
            [](const auto &injector) { return get_router(injector); }),
        di::bind<consensus::BlockExecutor>.to(
            [](auto const &inj) { return get_block_executor(inj); }),
        di::bind<consensus::grandpa::RoundObserver>.template to<consensus::grandpa::GrandpaImpl>(),
        di::bind<consensus::grandpa::CatchUpObserver>.template to<consensus::grandpa::GrandpaImpl>(),
        di::bind<consensus::grandpa::GrandpaObserver>.template to<consensus::grandpa::GrandpaImpl>(),
        di::bind<consensus::grandpa::Grandpa>.template to<consensus::grandpa::GrandpaImpl>(),
        di::bind<consensus::BabeUtil>.template to<consensus::BabeUtilImpl>(),

        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }

}  // namespace kagome::injector

#endif  // KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP
