/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "injector/application_injector.hpp"

#define BOOST_DI_CFG_DIAGNOSTICS_LEVEL 2

#include <boost/di.hpp>
#include <boost/di/extension/scopes/shared.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/injector/kademlia_injector.hpp>
#include <libp2p/log/configurator.hpp>

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
#include "clock/impl/basic_waitable_timer.hpp"
#include "clock/impl/clock_impl.hpp"
#include "clock/impl/ticker_impl.hpp"
#include "common/outcome_throw.hpp"
#include "consensus/authority/authority_manager.hpp"
#include "consensus/authority/authority_update_observer.hpp"
#include "consensus/authority/impl/authority_manager_impl.hpp"
#include "consensus/authority/impl/schedule_node.hpp"
#include "consensus/babe/impl/babe_impl.hpp"
#include "consensus/babe/impl/babe_lottery_impl.hpp"
#include "consensus/babe/impl/babe_synchronizer_impl.hpp"
#include "consensus/babe/impl/babe_util_impl.hpp"
#include "consensus/babe/impl/block_executor.hpp"
#include "consensus/grandpa/impl/environment_impl.hpp"
#include "consensus/grandpa/impl/grandpa_impl.hpp"
#include "consensus/validation/babe_block_validator.hpp"
#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/crypto_store/crypto_store_impl.hpp"
#include "crypto/crypto_store/session_keys.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "crypto/vrf/vrf_provider_impl.hpp"
#include "host_api/impl/host_api_factory_impl.hpp"
#include "log/configurator.hpp"
#include "log/logger.hpp"
#include "metrics/impl/exposer_impl.hpp"
#include "metrics/impl/prometheus/handler_impl.hpp"
#include "metrics/metrics.hpp"
#include "network/impl/block_announce_transmitter_impl.hpp"
#include "network/impl/extrinsic_observer_impl.hpp"
#include "network/impl/grandpa_transmitter_impl.hpp"
#include "network/impl/kademlia_storage_backend.hpp"
#include "network/impl/peer_manager_impl.hpp"
#include "network/impl/router_libp2p.hpp"
#include "network/impl/sync_protocol_observer_impl.hpp"
#include "network/impl/transactions_transmitter_impl.hpp"
#include "network/sync_protocol_observer.hpp"
#include "network/types/sync_clients_set.hpp"
#include "outcome/outcome.hpp"
#include "runtime/binaryen/binaryen_wasm_memory_factory.hpp"
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
#include "storage/database_error.hpp"
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

namespace {
  template <class T>
  using sptr = std::shared_ptr<T>;

  template <class T>
  using uptr = std::unique_ptr<T>;

  namespace di = boost::di;
  namespace fs = boost::filesystem;
  using namespace kagome;  // NOLINT

  template <typename C>
  auto useConfig(C c) {
    return boost::di::bind<std::decay_t<C>>().template to(
        std::move(c))[boost::di::override];
  }

  sptr<api::HttpListenerImpl> get_jrpc_api_http_listener(
      application::AppConfiguration const &config,
      sptr<application::AppStateManager> app_state_manager,
      sptr<api::RpcContext> context,
      api::HttpSession::Configuration http_session_config) {
    static auto initialized =
        boost::optional<sptr<api::HttpListenerImpl>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    auto &endpoint = config.rpcHttpEndpoint();

    api::HttpListenerImpl::Configuration listener_config;
    listener_config.endpoint = endpoint;

    auto listener = std::make_shared<api::HttpListenerImpl>(
        app_state_manager, context, listener_config, http_session_config);

    initialized.emplace(std::move(listener));
    return initialized.value();
  }

  sptr<api::WsListenerImpl> get_jrpc_api_ws_listener(
      application::AppConfiguration const &app_config,
      api::WsSession::Configuration ws_session_config,
      sptr<api::RpcContext> context,
      sptr<application::AppStateManager> app_state_manager) {
    static auto initialized =
        boost::optional<sptr<api::WsListenerImpl>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    api::WsListenerImpl::Configuration listener_config;
    listener_config.endpoint = app_config.rpcWsEndpoint();
    listener_config.ws_max_connections = app_config.maxWsConnections();

    auto listener =
        std::make_shared<api::WsListenerImpl>(app_state_manager,
                                              context,
                                              listener_config,
                                              std::move(ws_session_config));

    initialized.emplace(std::move(listener));
    return initialized.value();
  }

  sptr<blockchain::BlockStorage> get_block_storage(
      sptr<crypto::Hasher> hasher,
      sptr<storage::BufferStorage> db,
      sptr<storage::trie::TrieStorage> trie_storage,
      sptr<runtime::GrandpaApi> grandpa_api) {
    static auto initialized =
        boost::optional<sptr<blockchain::BlockStorage>>(boost::none);

    if (initialized) {
      return initialized.value();
    }

    auto storage_res = blockchain::KeyValueBlockStorage::create(
        trie_storage->getRootHash(),
        db,
        hasher,
        [&](const primitives::Block &genesis_block) {
          auto log = log::createLogger("Injector", "injector");

          auto res = db->get(authority::AuthorityManagerImpl::SCHEDULER_TREE);
          if (not res.has_value()
              && res == outcome::failure(storage::DatabaseError::NOT_FOUND)) {
            auto hash_res = db->get(storage::kGenesisBlockHashLookupKey);
            if (not hash_res.has_value()) {
              log->critical("Can't decode genesis block hash: {}",
                            hash_res.error().message());
              common::raise(hash_res.error());
            }

            primitives::BlockHash hash;
            std::copy(
                hash_res.value().begin(), hash_res.value().end(), hash.begin());

            // Get initial authorities from genesis
            auto authorities_res = grandpa_api->authorities(hash);
            if (not authorities_res.has_value()) {
              log->critical("Can't get genesis grandpa authorities: {}",
                            authorities_res.error().message());
              common::raise(authorities_res.error());
            }
            auto &authorities = authorities_res.value();
            authorities.id = 0;

            auto node = authority::ScheduleNode::createAsRoot({0, hash});
            node->actual_authorities =
                std::make_shared<primitives::AuthorityList>(
                    std::move(authorities));

            auto data_res = scale::encode(node);
            if (!data_res.has_value()) {
              log->critical("Can't encode authority manager state: {}",
                            data_res.error().message());
              common::raise(data_res.error());
            }

            auto save_res =
                db->put(authority::AuthorityManagerImpl::SCHEDULER_TREE,
                        common::Buffer(data_res.value()));
            if (!save_res.has_value()) {
              log->critical("Can't store current state: {}",
                            save_res.error().message());
              common::raise(save_res.error());
            }
          }
        });
    if (storage_res.has_error()) {
      common::raise(storage_res.error());
    }
    auto &storage = storage_res.value();

    initialized.emplace(std::move(storage));
    return initialized.value();
  }

  sptr<host_api::HostApiFactoryImpl> get_host_api_factory(
      sptr<storage::changes_trie::ChangesTracker> tracker,
      sptr<crypto::Sr25519Provider> sr25519_provider,
      sptr<crypto::Ed25519Provider> ed25519_provider,
      sptr<crypto::Secp256k1Provider> secp256k1_provider,
      sptr<crypto::Hasher> hasher,
      sptr<crypto::CryptoStore> crypto_store,
      sptr<crypto::Bip39Provider> bip39_provider) {
    static auto initialized =
        boost::optional<sptr<host_api::HostApiFactoryImpl>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    auto factory =
        std::make_shared<host_api::HostApiFactoryImpl>(tracker,
                                                       sr25519_provider,
                                                       ed25519_provider,
                                                       secp256k1_provider,
                                                       hasher,
                                                       crypto_store,
                                                       bip39_provider);

    initialized.emplace(std::move(factory));
    return initialized.value();
  }

  sptr<storage::trie::TrieStorageBackendImpl> get_trie_storage_backend(
      sptr<storage::BufferStorage> storage) {
    static auto initialized =
        boost::optional<sptr<storage::trie::TrieStorageBackendImpl>>(
            boost::none);

    if (initialized) {
      return initialized.value();
    }

    auto backend = std::make_shared<storage::trie::TrieStorageBackendImpl>(
        storage, common::Buffer{blockchain::prefix::TRIE_NODE});

    initialized.emplace(std::move(backend));
    return initialized.value();
  }

  sptr<storage::trie::TrieStorageImpl> get_trie_storage_impl(
      sptr<storage::trie::PolkadotTrieFactory> factory,
      sptr<storage::trie::Codec> codec,
      sptr<storage::trie::TrieSerializer> serializer,
      sptr<storage::changes_trie::ChangesTracker> tracker) {
    static auto initialized =
        boost::optional<sptr<storage::trie::TrieStorageImpl>>(boost::none);

    if (initialized) {
      return initialized.value();
    }

    auto trie_storage_res = storage::trie::TrieStorageImpl::createEmpty(
        factory, codec, serializer, tracker);

    if (!trie_storage_res) {
      common::raise(trie_storage_res.error());
    }
    auto &trie_storage = trie_storage_res.value();

    initialized.emplace(std::move(trie_storage));
    return initialized.value();
  }

  sptr<storage::trie::TrieStorage> get_trie_storage(
      sptr<application::ChainSpec> configuration_storage,
      sptr<storage::trie::TrieStorageImpl> trie_storage) {
    static auto initialized =
        boost::optional<sptr<storage::trie::TrieStorage>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    auto batch = trie_storage->getPersistentBatch();
    if (not batch) {
      common::raise(batch.error());
    }

    auto log = log::createLogger("Injector", "injector");

    const auto &genesis_raw_configs = configuration_storage->getGenesis();

    for (const auto &[key_, val_] : genesis_raw_configs) {
      auto &key = key_;
      auto &val = val_;
      SL_DEBUG(
          log, "Key: {}, Val: {}", key.toHex(), val.toHex().substr(0, 200));
      if (auto res = batch.value()->put(key, val); not res) {
        common::raise(res.error());
      }
    }
    if (auto res = batch.value()->commit(); not res) {
      common::raise(res.error());
    }

    initialized.emplace(std::move(trie_storage));
    return initialized.value();
  }

  sptr<storage::BufferStorage> get_level_db(
      application::AppConfiguration const &app_config,
      sptr<application::ChainSpec> chain_spec) {
    static auto initialized =
        boost::optional<sptr<storage::BufferStorage>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    auto options = leveldb::Options{};
    options.create_if_missing = true;
    auto db_res = storage::LevelDB::create(
        app_config.databasePath(chain_spec->id()), options);
    if (!db_res) {
      auto log = log::createLogger("Injector", "injector");
      log->critical("Can't create LevelDB in {}: {}",
                    fs::absolute(app_config.databasePath(chain_spec->id()),
                                 fs::current_path())
                        .native(),
                    db_res.error().message());
      exit(EXIT_FAILURE);
    }
    auto &db = db_res.value();

    initialized.emplace(std::move(db));
    return initialized.value();
  }

  std::shared_ptr<application::ChainSpec> get_chain_spec(
      application::AppConfiguration const &config) {
    static auto initialized =
        boost::optional<sptr<application::ChainSpec>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    auto const &chainspec_path = config.chainSpecPath();

    auto chain_spec_res =
        application::ChainSpecImpl::loadFrom(chainspec_path.native());
    if (not chain_spec_res.has_value()) {
      auto log = log::createLogger("Injector", "injector");
      log->critical(
          "Can't load chain spec from {}: {}",
          fs::absolute(chainspec_path.native(), fs::current_path()).native(),
          chain_spec_res.error().message());
      exit(EXIT_FAILURE);
    }
    auto &chain_spec = chain_spec_res.value();

    initialized.emplace(std::move(chain_spec));
    return initialized.value();
  }

  sptr<primitives::BabeConfiguration> get_babe_configuration(
      sptr<runtime::BabeApi> babe_api) {
    static auto initialized =
        boost::optional<sptr<primitives::BabeConfiguration>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    auto configuration_res = babe_api->configuration();
    if (not configuration_res) {
      common::raise(configuration_res.error());
    }

    auto configuration = std::make_shared<primitives::BabeConfiguration>(
        std::move(configuration_res.value()));

    auto log = log::createLogger("Injector", "injector");
    for (const auto &authority : configuration->genesis_authorities) {
      SL_DEBUG(log, "Babe authority: {}", authority.id.id.toHex());
    }
    configuration->leadership_rate.first *= 3;

    initialized.emplace(std::move(configuration));
    return initialized.value();
  }

  sptr<crypto::KeyFileStorage> get_key_file_storage(
      application::AppConfiguration const &config,
      sptr<application::ChainSpec> chain_spec) {
    static boost::optional<sptr<crypto::KeyFileStorage>> initialized =
        boost::none;
    static boost::optional<fs::path> initialized_path = boost::none;

    auto path = config.keystorePath(chain_spec->id());
    if (initialized and initialized_path and initialized_path.value() == path) {
      return initialized.value();
    }
    auto key_file_storage_res = crypto::KeyFileStorage::createAt(path);
    if (not key_file_storage_res) {
      common::raise(key_file_storage_res.error());
    }

    initialized = std::move(key_file_storage_res.value());
    initialized_path = std::move(path);

    return initialized.value();
  }

  const sptr<libp2p::crypto::KeyPair> &get_peer_keypair(
      const application::AppConfiguration &app_config,
      const crypto::Ed25519Provider &crypto_provider,
      const crypto::CryptoStore &crypto_store) {
    static auto initialized =
        boost::optional<sptr<libp2p::crypto::KeyPair>>(boost::none);

    if (initialized) {
      return initialized.value();
    }

    auto log = log::createLogger("Injector", "injector");

    if (app_config.nodeKey()) {
      log->info("Will use LibP2P keypair from config or 'node-key' CLI arg");

      auto provided_keypair =
          crypto_provider.generateKeypair(app_config.nodeKey().value());
      BOOST_ASSERT(provided_keypair.secret_key == app_config.nodeKey().value());

      auto &&pub = provided_keypair.public_key;
      auto &&priv = provided_keypair.secret_key;

      auto key_pair =
          std::make_shared<libp2p::crypto::KeyPair>(libp2p::crypto::KeyPair{
              .publicKey = {{.type = libp2p::crypto::Key::Type::Ed25519,
                             .data = {pub.begin(), pub.end()}}},
              .privateKey = {{.type = libp2p::crypto::Key::Type::Ed25519,
                              .data = {priv.begin(), priv.end()}}}});

      initialized.emplace(std::move(key_pair));
      return initialized.value();
    }

    if (app_config.nodeKeyFile()) {
      const auto &path = app_config.nodeKeyFile().value();
      log->info(
          "Will use LibP2P keypair from config or 'node-key-file' CLI arg");
      auto key = crypto_store.loadLibp2pKeypair(path);
      if (key.has_error()) {
        log->error("Unable to load user provided key from {}. Error: {}",
                   path,
                   key.error().message());
      } else {
        auto key_pair =
            std::make_shared<libp2p::crypto::KeyPair>(std::move(key.value()));
        initialized.emplace(std::move(key_pair));
        return initialized.value();
      }
    }

    if (crypto_store.getLibp2pKeypair().has_value()) {
      log->info(
          "Will use LibP2P keypair from config or args (loading from base "
          "path)");

      auto stored_keypair = crypto_store.getLibp2pKeypair().value();

      auto key_pair =
          std::make_shared<libp2p::crypto::KeyPair>(std::move(stored_keypair));

      initialized.emplace(std::move(key_pair));
      return initialized.value();
    }

    log->warn(
        "Can not obtain a libp2p keypair from crypto storage. "
        "A unique one will be generated for the current session");

    auto generated_keypair = crypto_provider.generateKeypair();

    auto &&pub = generated_keypair.public_key;
    auto &&priv = generated_keypair.secret_key;

    auto key_pair =
        std::make_shared<libp2p::crypto::KeyPair>(libp2p::crypto::KeyPair{
            .publicKey = {{.type = libp2p::crypto::Key::Type::Ed25519,
                           .data = {pub.begin(), pub.end()}}},
            .privateKey = {{.type = libp2p::crypto::Key::Type::Ed25519,
                            .data = {priv.begin(), priv.end()}}}});

    initialized.emplace(std::move(key_pair));
    return initialized.value();
  }

  sptr<libp2p::protocol::kademlia::Config> get_kademlia_config(
      const application::ChainSpec &chain_spec) {
    static auto initialized =
        boost::optional<sptr<libp2p::protocol::kademlia::Config>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    auto kagome_config = std::make_shared<libp2p::protocol::kademlia::Config>(
        libp2p::protocol::kademlia::Config{
            .protocolId = "/" + chain_spec.protocolId() + "/kad",
            .maxBucketSize = 1000,
            .randomWalk = {.interval = std::chrono::minutes(1)}});

    initialized.emplace(std::move(kagome_config));
    return initialized.value();
  }

  template <typename Injector>
  sptr<api::ApiServiceImpl> get_jrpc_api_service(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<api::ApiServiceImpl>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    auto asmgr =
        injector
            .template create<std::shared_ptr<application::AppStateManager>>();
    auto thread_pool = injector.template create<sptr<api::RpcThreadPool>>();
    auto server = injector.template create<sptr<api::JRpcServer>>();
    auto listeners =
        injector.template create<api::ApiServiceImpl::ListenerList>();
    auto processors =
        injector.template create<api::ApiServiceImpl::ProcessorSpan>();
    auto storage_sub_engine = injector.template create<
        primitives::events::StorageSubscriptionEnginePtr>();
    auto chain_sub_engine =
        injector
            .template create<primitives::events::ChainSubscriptionEnginePtr>();
    auto ext_sub_engine = injector.template create<
        primitives::events::ExtrinsicSubscriptionEnginePtr>();
    auto extrinsic_event_key_repo =
        injector
            .template create<sptr<subscription::ExtrinsicEventKeyRepository>>();
    auto block_tree = injector.template create<sptr<blockchain::BlockTree>>();
    auto trie_storage =
        injector.template create<sptr<storage::trie::TrieStorage>>();

    auto api_service =
        std::make_shared<api::ApiServiceImpl>(asmgr,
                                              thread_pool,
                                              listeners,
                                              server,
                                              processors,
                                              storage_sub_engine,
                                              chain_sub_engine,
                                              ext_sub_engine,
                                              extrinsic_event_key_repo,
                                              block_tree,
                                              trie_storage);

    auto state_api = injector.template create<std::shared_ptr<api::StateApi>>();
    state_api->setApiService(api_service);

    auto chain_api = injector.template create<std::shared_ptr<api::ChainApi>>();
    chain_api->setApiService(api_service);

    auto author_api =
        injector.template create<std::shared_ptr<api::AuthorApi>>();
    author_api->setApiService(api_service);

    initialized.emplace(std::move(api_service));
    return initialized.value();
  }

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

    auto block_tree_res =
        blockchain::BlockTreeImpl::create(std::move(header_repo),
                                          std::move(storage),
                                          std::move(block_id),
                                          std::move(extrinsic_observer),
                                          std::move(hasher),
                                          std::move(chain_events_engine),
                                          std::move(ext_events_engine),
                                          std::move(ext_events_key_repo),
                                          std::move(runtime_core),
                                          std::move(babe_configuration),
                                          std::move(babe_util));
    if (not block_tree_res.has_value()) {
      common::raise(block_tree_res.error());
    }
    auto &block_tree = block_tree_res.value();

    auto tagged_transaction_queue = injector.template create<
        std::shared_ptr<runtime::binaryen::TaggedTransactionQueueImpl>>();
    tagged_transaction_queue->setBlockTree(block_tree);

    auto protocol_factory =
        injector.template create<std::shared_ptr<network::ProtocolFactory>>();

    protocol_factory->setBlockTree(block_tree);

    initialized.emplace(std::move(block_tree));
    return initialized.value();
  }

  template <class Injector>
  sptr<network::PeerManager> get_peer_manager(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<network::PeerManager>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    auto peer_manager = std::make_shared<network::PeerManagerImpl>(
        injector.template create<sptr<application::AppStateManager>>(),
        injector.template create<libp2p::Host &>(),
        injector.template create<sptr<libp2p::protocol::Identify>>(),
        injector.template create<sptr<libp2p::protocol::kademlia::Kademlia>>(),
        injector.template create<sptr<libp2p::protocol::Scheduler>>(),
        injector.template create<sptr<network::StreamEngine>>(),
        injector.template create<const application::AppConfiguration &>(),
        injector.template create<sptr<clock::SteadyClock>>(),
        injector.template create<const network::BootstrapNodes &>(),
        injector.template create<const network::OwnPeerInfo &>(),
        injector.template create<sptr<network::SyncClientsSet>>(),
        injector.template create<sptr<network::Router>>(),
        injector.template create<sptr<storage::BufferStorage>>());

    auto protocol_factory =
        injector.template create<std::shared_ptr<network::ProtocolFactory>>();

    protocol_factory->setPeerManager(peer_manager);

    initialized.emplace(std::move(peer_manager));
    return initialized.value();
  }

  template <typename Injector>
  sptr<consensus::BlockExecutor> get_block_executor(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<consensus::BlockExecutor>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    auto block_executor = std::make_shared<consensus::BlockExecutor>(
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

    initialized.emplace(std::move(block_executor));
    return initialized.value();
  }

  template <typename Injector>
  sptr<network::SyncProtocolObserverImpl> get_sync_observer_impl(
      const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<network::SyncProtocolObserverImpl>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    auto sync_observer = std::make_shared<network::SyncProtocolObserverImpl>(
        injector.template create<sptr<blockchain::BlockTree>>(),
        injector.template create<sptr<blockchain::BlockHeaderRepository>>());

    auto protocol_factory =
        injector.template create<std::shared_ptr<network::ProtocolFactory>>();

    protocol_factory->setSyncObserver(sync_observer);

    initialized.emplace(std::move(sync_observer));
    return initialized.value();
  }

  template <typename... Ts>
  auto makeApplicationInjector(const application::AppConfiguration &config,
                               Ts &&... args) {
    // default values for configurations
    api::RpcThreadPool::Configuration rpc_thread_pool_config{};
    api::HttpSession::Configuration http_config{};
    api::WsSession::Configuration ws_config{};
    transaction_pool::PoolModeratorImpl::Params pool_moderator_config{};
    transaction_pool::TransactionPool::Limits tp_pool_limits{};
    libp2p::protocol::PingConfig ping_config{};

    return di::make_injector(
        // bind configs
        useConfig(rpc_thread_pool_config),
        useConfig(http_config),
        useConfig(ws_config),
        useConfig(pool_moderator_config),
        useConfig(tp_pool_limits),
        useConfig(ping_config),

        // inherit host injector
        libp2p::injector::makeHostInjector(
            libp2p::injector::useSecurityAdaptors<
                libp2p::security::Noise>()[di::override]),

        // inherit kademlia injector
        libp2p::injector::makeKademliaInjector(),
        di::bind<libp2p::protocol::kademlia::Config>.to(
            [](auto const &injector) {
              auto &chain_spec =
                  injector.template create<application::ChainSpec &>();
              return get_kademlia_config(chain_spec);
            })[boost::di::override],

        di::bind<application::AppStateManager>.template to<application::AppStateManagerImpl>(),
        di::bind<application::AppConfiguration>.to(config),

        // compose peer keypair
        di::bind<libp2p::crypto::KeyPair>.to([](auto const &injector) {
          auto &app_config =
              injector.template create<const application::AppConfiguration &>();
          auto &crypto_provider =
              injector.template create<const crypto::Ed25519Provider &>();
          auto &crypto_store =
              injector.template create<const crypto::CryptoStore &>();
          return get_peer_keypair(app_config, crypto_provider, crypto_store);
        })[boost::di::override],

        // bind io_context: 1 per injector
        di::bind<::boost::asio::io_context>.in(
            di::extension::shared)[boost::di::override],

        di::bind<api::ApiServiceImpl::ListenerList>.to([](auto const
                                                              &injector) {
          std::vector<std::shared_ptr<api::Listener>> listeners{
              injector
                  .template create<std::shared_ptr<api::HttpListenerImpl>>(),
              injector.template create<std::shared_ptr<api::WsListenerImpl>>(),
          };
          return api::ApiServiceImpl::ListenerList{std::move(listeners)};
        }),
        di::bind<api::ApiServiceImpl::ProcessorSpan>.to([](auto const
                                                               &injector) {
          static std::vector<std::shared_ptr<api::JRpcProcessor>> processors{
              injector.template create<
                  std::shared_ptr<api::state::StateJrpcProcessor>>(),
              injector.template create<
                  std::shared_ptr<api::author::AuthorJRpcProcessor>>(),
              injector.template create<
                  std::shared_ptr<api::chain::ChainJrpcProcessor>>(),
              injector.template create<
                  std::shared_ptr<api::system::SystemJrpcProcessor>>(),
              injector.template create<
                  std::shared_ptr<api::rpc::RpcJRpcProcessor>>(),
              injector.template create<
                  std::shared_ptr<api::payment::PaymentJRpcProcessor>>()};
          return api::ApiServiceImpl::ProcessorSpan{processors};
        }),
        // bind interfaces
        di::bind<api::HttpListenerImpl>.to([](const auto &injector) {
          const application::AppConfiguration &config =
              injector.template create<application::AppConfiguration const &>();
          auto app_state_manager =
              injector.template create<sptr<application::AppStateManager>>();
          auto context = injector.template create<sptr<api::RpcContext>>();
          auto &&http_session_config =
              injector.template create<api::HttpSession::Configuration>();

          return get_jrpc_api_http_listener(
              config, app_state_manager, context, http_session_config);
        }),
        di::bind<api::WsListenerImpl>.to([](const auto &injector) {
          auto config =
              injector.template create<api::WsSession::Configuration>();
          auto context = injector.template create<sptr<api::RpcContext>>();
          auto app_state_manager =
              injector.template create<sptr<application::AppStateManager>>();
          const application::AppConfiguration &app_config =
              injector.template create<application::AppConfiguration const &>();
          return get_jrpc_api_ws_listener(
              app_config, config, context, app_state_manager);
        }),
        // starting metrics interfaces
        di::bind<metrics::Handler>.template to<metrics::PrometheusHandler>(),
        di::bind<metrics::Exposer>.template to<metrics::ExposerImpl>(),
        di::bind<metrics::Exposer::Configuration>.to([](const auto &injector) {
          return metrics::Exposer::Configuration{
              injector.template create<application::AppConfiguration const &>()
                  .openmetricsHttpEndpoint()};
        }),
        // hardfix for Mac clang
        di::bind<metrics::Session::Configuration>.to([](const auto &injector) {
          return metrics::Session::Configuration{};
        }),
        // ending metrics interfaces
        di::bind<libp2p::crypto::random::RandomGenerator>.template to<libp2p::crypto::random::BoostRandomGenerator>()
            [di::override],
        di::bind<api::AuthorApi>.template to<api::AuthorApiImpl>(),
        di::bind<crypto::SessionKeys>.template to<crypto::SessionKeys>(),
        di::bind<network::Roles>.to(config.roles()),
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
        di::bind<storage::BufferStorage>.to([](const auto &injector) {
          const application::AppConfiguration &config =
              injector.template create<application::AppConfiguration const &>();
          auto chain_spec =
              injector.template create<sptr<application::ChainSpec>>();
          return get_level_db(config, chain_spec);
        }),
        di::bind<blockchain::BlockStorage>.to([](const auto &injector) {
          const auto &hasher = injector.template create<sptr<crypto::Hasher>>();
          const auto &db =
              injector.template create<sptr<storage::BufferStorage>>();
          const auto &trie_storage =
              injector.template create<sptr<storage::trie::TrieStorage>>();
          const auto &grandpa_api =
              injector.template create<sptr<runtime::GrandpaApi>>();
          return get_block_storage(hasher, db, trie_storage, grandpa_api);
        }),
        di::bind<blockchain::BlockTree>.to(
            [](auto const &injector) { return get_block_tree(injector); }),
        di::bind<blockchain::BlockHeaderRepository>.template to<blockchain::KeyValueBlockHeaderRepository>(),
        di::bind<clock::SystemClock>.template to<clock::SystemClockImpl>(),
        di::bind<clock::SteadyClock>.template to<clock::SteadyClockImpl>(),
        di::bind<clock::Timer>.template to<clock::BasicWaitableTimer>(),
        di::bind<clock::Ticker>.template to<clock::TickerImpl>(),
        di::bind<clock::SystemClock::Duration>.to([](const auto &injector) {
          auto conf =
              injector.template create<sptr<primitives::BabeConfiguration>>();
          return conf->slot_duration;
        }),
        di::bind<primitives::BabeConfiguration>.to([](auto const &injector) {
          auto babe_api = injector.template create<sptr<runtime::BabeApi>>();
          return get_babe_configuration(babe_api);
        }),
        di::bind<consensus::BabeSynchronizer>.template to<consensus::BabeSynchronizerImpl>(),
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
          const application::AppConfiguration &config =
              injector.template create<application::AppConfiguration const &>();
          auto chain_spec =
              injector.template create<sptr<application::ChainSpec>>();

          return get_key_file_storage(config, chain_spec);
        }),
        di::bind<crypto::CryptoStore>.template to<crypto::CryptoStoreImpl>(),
        di::bind<host_api::HostApiFactory>.template to(
            [](auto const &injector) {
              auto tracker = injector.template create<
                  sptr<storage::changes_trie::ChangesTracker>>();
              auto sr25519_provider =
                  injector.template create<sptr<crypto::Sr25519Provider>>();
              auto ed25519_provider =
                  injector.template create<sptr<crypto::Ed25519Provider>>();
              auto secp256k1_provider =
                  injector.template create<sptr<crypto::Secp256k1Provider>>();
              auto hasher = injector.template create<sptr<crypto::Hasher>>();
              auto crypto_store =
                  injector.template create<sptr<crypto::CryptoStore>>();
              auto bip39_provider =
                  injector.template create<sptr<crypto::Bip39Provider>>();

              return get_host_api_factory(tracker,
                                          sr25519_provider,
                                          ed25519_provider,
                                          secp256k1_provider,
                                          hasher,
                                          crypto_store,
                                          bip39_provider);
            }),
        di::bind<network::SyncProtocolObserver>.to([](auto const &injector) {
          return get_sync_observer_impl(injector);
        }),
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
            [](auto const &injector) {
              auto storage =
                  injector.template create<sptr<storage::BufferStorage>>();
              return get_trie_storage_backend(storage);
            }),
        di::bind<storage::trie::TrieStorageImpl>.to([](auto const &injector) {
          auto factory =
              injector
                  .template create<sptr<storage::trie::PolkadotTrieFactory>>();
          auto codec = injector.template create<sptr<storage::trie::Codec>>();
          auto serializer =
              injector.template create<sptr<storage::trie::TrieSerializer>>();
          auto tracker = injector.template create<
              sptr<storage::changes_trie::ChangesTracker>>();
          return get_trie_storage_impl(factory, codec, serializer, tracker);
        }),
        di::bind<storage::trie::TrieStorage>.to([](auto const &injector) {
          auto configuration_storage =
              injector.template create<sptr<application::ChainSpec>>();
          auto trie_storage =
              injector.template create<sptr<storage::trie::TrieStorageImpl>>();
          return get_trie_storage(configuration_storage, trie_storage);
        }),
        di::bind<storage::trie::PolkadotTrieFactory>.template to<storage::trie::PolkadotTrieFactoryImpl>(),
        di::bind<storage::trie::Codec>.template to<storage::trie::PolkadotCodec>(),
        di::bind<storage::trie::TrieSerializer>.template to<storage::trie::TrieSerializerImpl>(),
        di::bind<runtime::WasmProvider>.template to<runtime::StorageWasmProvider>(),
        di::bind<application::ChainSpec>.to([](const auto &injector) {
          const application::AppConfiguration &config =
              injector.template create<application::AppConfiguration const &>();
          return get_chain_spec(config);
        }),
        di::bind<network::ExtrinsicObserver>.to([](const auto &injector) {
          return get_extrinsic_observer_impl(injector);
        }),
        di::bind<authority::AuthorityUpdateObserver>.template to<authority::AuthorityManagerImpl>(),
        di::bind<authority::AuthorityManager>.template to<authority::AuthorityManagerImpl>(),
        di::bind<network::PeerManager>.to(
            [](auto const &injector) { return get_peer_manager(injector); }),
        di::bind<network::Router>.template to<network::RouterLibp2p>(),
        di::bind<consensus::BlockExecutor>.to(
            [](auto const &injector) { return get_block_executor(injector); }),
        di::bind<consensus::grandpa::Grandpa>.to(
            [](auto const &injector) { return get_grandpa_impl(injector); }),
        di::bind<consensus::grandpa::RoundObserver>.to(
            [](auto const &injector) { return get_grandpa_impl(injector); }),
        di::bind<consensus::grandpa::CatchUpObserver>.to(
            [](auto const &injector) { return get_grandpa_impl(injector); }),
        di::bind<consensus::grandpa::NeighborObserver>.to(
            [](auto const &injector) { return get_grandpa_impl(injector); }),
        di::bind<consensus::grandpa::GrandpaObserver>.to(
            [](auto const &injector) { return get_grandpa_impl(injector); }),
        di::bind<consensus::BabeUtil>.template to<consensus::BabeUtilImpl>(),
        di::bind<network::BlockAnnounceTransmitter>.template to<network::BlockAnnounceTransmitterImpl>(),
        di::bind<network::GrandpaTransmitter>.template to<network::GrandpaTransmitterImpl>(),
        di::bind<network::TransactionsTransmitter>.template to<network::TransactionsTransmitterImpl>(),

        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }

  template <typename Injector>
  sptr<network::OwnPeerInfo> get_own_peer_info(const Injector &injector) {
    static boost::optional<sptr<network::OwnPeerInfo>> initialized{boost::none};
    if (initialized) {
      return initialized.value();
    }

    libp2p::crypto::PublicKey public_key;

    const auto &config =
        injector.template create<application::AppConfiguration const &>();

    if (config.roles().flags.authority) {
      auto &crypto_provider =
          injector.template create<const crypto::Ed25519Provider &>();
      auto &crypto_store =
          injector.template create<const crypto::CryptoStore &>();

      auto &local_pair =
          get_peer_keypair(config, crypto_provider, crypto_store);

      public_key = local_pair->publicKey;
    } else {
      auto &&local_pair = injector.template create<libp2p::crypto::KeyPair>();
      public_key = local_pair.publicKey;
    }

    auto &key_marshaller =
        injector.template create<libp2p::crypto::marshaller::KeyMarshaller &>();

    libp2p::peer::PeerId peer_id =
        libp2p::peer::PeerId::fromPublicKey(
            key_marshaller.marshal(public_key).value())
            .value();

    std::vector<libp2p::multi::Multiaddress> listen_addrs =
        config.listenAddresses();
    std::vector<libp2p::multi::Multiaddress> public_addrs =
        config.publicAddresses();

    auto log = log::createLogger("Injector", "injector");
    for (auto &addr : listen_addrs) {
      SL_DEBUG(log, "Peer listening on multiaddr: {}", addr.getStringAddress());
    }
    for (auto &addr : public_addrs) {
      SL_DEBUG(log, "Peer public multiaddr: {}", addr.getStringAddress());
    }

    initialized = std::make_shared<network::OwnPeerInfo>(
        std::move(peer_id), std::move(public_addrs), std::move(listen_addrs));
    return initialized.value();
  }

  template <typename Injector>
  sptr<consensus::babe::Babe> get_babe(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<consensus::babe::Babe>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    auto session_keys = injector.template create<sptr<crypto::SessionKeys>>();

    initialized = std::make_shared<consensus::babe::BabeImpl>(
        injector.template create<sptr<application::AppStateManager>>(),
        injector.template create<sptr<consensus::BabeLottery>>(),
        injector.template create<sptr<consensus::BlockExecutor>>(),
        injector.template create<sptr<storage::trie::TrieStorage>>(),
        injector.template create<sptr<primitives::BabeConfiguration>>(),
        injector.template create<sptr<authorship::Proposer>>(),
        injector.template create<sptr<blockchain::BlockTree>>(),
        injector.template create<sptr<network::BlockAnnounceTransmitter>>(),
        injector.template create<sptr<crypto::Sr25519Provider>>(),
        session_keys->getBabeKeyPair(),
        injector.template create<sptr<clock::SystemClock>>(),
        injector.template create<sptr<crypto::Hasher>>(),
        injector.template create<uptr<clock::Ticker>>(),
        injector.template create<sptr<authority::AuthorityUpdateObserver>>(),
        injector.template create<sptr<consensus::BabeUtil>>());

    auto protocol_factory =
        injector.template create<std::shared_ptr<network::ProtocolFactory>>();

    protocol_factory->setBabe(initialized.value());

    return initialized.value();
  }

  template <typename Injector>
  sptr<network::ExtrinsicObserverImpl> get_extrinsic_observer_impl(
      const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<network::ExtrinsicObserverImpl>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    initialized = std::make_shared<network::ExtrinsicObserverImpl>(
        injector.template create<sptr<api::AuthorApi>>());

    auto protocol_factory =
        injector.template create<std::shared_ptr<network::ProtocolFactory>>();

    protocol_factory->setExtrinsicObserver(initialized.value());

    return initialized.value();
  }

  template <typename Injector>
  sptr<consensus::grandpa::GrandpaImpl> get_grandpa_impl(
      const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<consensus::grandpa::GrandpaImpl>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    auto session_keys = injector.template create<sptr<crypto::SessionKeys>>();

    initialized = std::make_shared<consensus::grandpa::GrandpaImpl>(
        injector.template create<sptr<application::AppStateManager>>(),
        injector.template create<sptr<consensus::grandpa::Environment>>(),
        injector.template create<sptr<storage::BufferStorage>>(),
        injector.template create<sptr<crypto::Ed25519Provider>>(),
        injector.template create<sptr<runtime::GrandpaApi>>(),
        session_keys->getGranKeyPair(),
        injector.template create<sptr<clock::SteadyClock>>(),
        injector.template create<sptr<boost::asio::io_context>>(),
        injector.template create<sptr<authority::AuthorityManager>>(),
        injector.template create<sptr<consensus::babe::Babe>>());

    auto protocol_factory =
        injector.template create<std::shared_ptr<network::ProtocolFactory>>();

    protocol_factory->setGrandpaObserver(initialized.value());

    return initialized.value();
  }

  template <typename... Ts>
  auto makeKagomeNodeInjector(const application::AppConfiguration &app_config,
                              Ts &&... args) {
    using namespace boost;  // NOLINT;

    return di::make_injector(
        makeApplicationInjector(app_config),
        // compose peer info
        di::bind<network::OwnPeerInfo>.to(
            [](const auto &injector) { return get_own_peer_info(injector); }),
        di::bind<consensus::babe::Babe>.to(
            [](auto const &injector) { return get_babe(injector); }),
        di::bind<consensus::BabeLottery>.template to<consensus::BabeLotteryImpl>(),
        di::bind<network::BabeObserver>.to(
            [](auto const &injector) { return get_babe(injector); }),
        di::bind<runtime::GrandpaApi>.template to<runtime::binaryen::GrandpaApiImpl>()
            [di::override],

        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }

}  // namespace

namespace kagome::injector {

  class KagomeNodeInjectorImpl {
   public:
    using Injector = decltype(makeKagomeNodeInjector(
        std::declval<application::AppConfiguration const &>()));

    KagomeNodeInjectorImpl(Injector injector)
        : injector_{std::move(injector)} {}
    Injector injector_;
  };

  KagomeNodeInjector::KagomeNodeInjector(
      const application::AppConfiguration &app_config)
      : pimpl_{std::make_unique<KagomeNodeInjectorImpl>(
          makeKagomeNodeInjector(app_config))} {}

  sptr<application::ChainSpec> KagomeNodeInjector::injectChainSpec() {
    return pimpl_->injector_.create<sptr<application::ChainSpec>>();
  }

  sptr<application::AppStateManager>
  KagomeNodeInjector::injectAppStateManager() {
    return pimpl_->injector_.create<sptr<application::AppStateManager>>();
  }

  sptr<boost::asio::io_context> KagomeNodeInjector::injectIoContext() {
    return pimpl_->injector_.create<sptr<boost::asio::io_context>>();
  }

  sptr<metrics::Exposer> KagomeNodeInjector::injectOpenMetricsService() {
    // registry here is temporary, it initiates static global registry
    // and registers handler in there
    auto registry = metrics::createRegistry();
    auto handler = pimpl_->injector_.create<sptr<metrics::Handler>>();
    registry->setHandler(*handler.get());
    auto exposer = pimpl_->injector_.create<sptr<metrics::Exposer>>();
    exposer->setHandler(handler);
    return exposer;
  }

  sptr<network::Router> KagomeNodeInjector::injectRouter() {
    return pimpl_->injector_.create<sptr<network::Router>>();
  }

  sptr<network::PeerManager> KagomeNodeInjector::injectPeerManager() {
    return pimpl_->injector_.create<sptr<network::PeerManager>>();
  }

  sptr<api::ApiService> KagomeNodeInjector::injectRpcApiService() {
    return pimpl_->injector_.create<sptr<api::ApiService>>();
  }

  std::shared_ptr<clock::SystemClock> KagomeNodeInjector::injectSystemClock() {
    return pimpl_->injector_.create<sptr<clock::SystemClock>>();
  }

  std::shared_ptr<network::SyncProtocolObserver>
  KagomeNodeInjector::injectSyncObserver() {
    return pimpl_->injector_.create<sptr<network::SyncProtocolObserver>>();
  }

  std::shared_ptr<consensus::babe::Babe> KagomeNodeInjector::injectBabe() {
    return pimpl_->injector_.create<sptr<consensus::babe::Babe>>();
  }

  std::shared_ptr<consensus::grandpa::Grandpa>
  KagomeNodeInjector::injectGrandpa() {
    return pimpl_->injector_.create<sptr<consensus::grandpa::Grandpa>>();
  }

  std::shared_ptr<soralog::LoggingSystem>
  KagomeNodeInjector::injectLoggingSystem() {
    return std::make_shared<soralog::LoggingSystem>(
        std::make_shared<kagome::log::Configurator>(
            pimpl_->injector_.create<sptr<libp2p::log::Configurator>>()));
  }

}  // namespace kagome::injector
