/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "injector/application_injector.hpp"

#define BOOST_DI_CFG_DIAGNOSTICS_LEVEL 2
#define BOOST_DI_CFG_CTOR_LIMIT_SIZE \
  16  // TODO(Harrm): check how it influences on compilation time

#include <rocksdb/filter_policy.h>
#include <rocksdb/table.h>
#include <boost/di.hpp>
#include <boost/di/extension/scopes/shared.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/injector/kademlia_injector.hpp>
#include <libp2p/log/configurator.hpp>

#undef U64  // comes from OpenSSL and messes with WAVM

#include "api/service/author/author_jrpc_processor.hpp"
#include "api/service/author/impl/author_api_impl.hpp"
#include "api/service/chain/chain_jrpc_processor.hpp"
#include "api/service/chain/impl/chain_api_impl.hpp"
#include "api/service/child_state/child_state_jrpc_processor.hpp"
#include "api/service/child_state/impl/child_state_api_impl.hpp"
#include "api/service/impl/api_service_impl.hpp"
#include "api/service/internal/impl/internal_api_impl.hpp"
#include "api/service/internal/internal_jrpc_processor.hpp"
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
#include "application/modes/print_chain_info_mode.hpp"
#include "application/modes/recovery_mode.hpp"
#include "authority_discovery/publisher/address_publisher.hpp"
#include "authorship/impl/block_builder_factory_impl.hpp"
#include "authorship/impl/block_builder_impl.hpp"
#include "authorship/impl/proposer_impl.hpp"
#include "blockchain/impl/block_header_repository_impl.hpp"
#include "blockchain/impl/block_storage_impl.hpp"
#include "blockchain/impl/block_tree_impl.hpp"
#include "blockchain/impl/digest_tracker_impl.hpp"
#include "blockchain/impl/justification_storage_policy.hpp"
#include "clock/impl/basic_waitable_timer.hpp"
#include "clock/impl/clock_impl.hpp"
#include "common/fd_limit.hpp"
#include "common/outcome_throw.hpp"
#include "consensus/babe/impl/babe_config_repository_impl.hpp"
#include "consensus/babe/impl/babe_impl.hpp"
#include "consensus/babe/impl/babe_lottery_impl.hpp"
#include "consensus/babe/impl/block_appender_base.hpp"
#include "consensus/babe/impl/block_executor_impl.hpp"
#include "consensus/babe/impl/block_header_appender_impl.hpp"
#include "consensus/babe/impl/consistency_keeper_impl.hpp"
#include "consensus/grandpa/impl/authority_manager_impl.hpp"
#include "consensus/grandpa/impl/environment_impl.hpp"
#include "consensus/grandpa/impl/grandpa_impl.hpp"
#include "consensus/validation/babe_block_validator.hpp"
#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/crypto_store/crypto_store_impl.hpp"
#include "crypto/crypto_store/session_keys.hpp"
#include "crypto/ecdsa/ecdsa_provider_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "crypto/vrf/vrf_provider_impl.hpp"
#include "host_api/impl/host_api_factory_impl.hpp"
#include "host_api/impl/host_api_impl.hpp"
#include "injector/calculate_genesis_state.hpp"
#include "injector/get_peer_keypair.hpp"
#include "log/configurator.hpp"
#include "log/logger.hpp"
#include "metrics/impl/exposer_impl.hpp"
#include "metrics/impl/metrics_watcher.hpp"
#include "metrics/impl/prometheus/handler_impl.hpp"
#include "metrics/metrics.hpp"
#include "network/impl/block_announce_transmitter_impl.hpp"
#include "network/impl/extrinsic_observer_impl.hpp"
#include "network/impl/grandpa_transmitter_impl.hpp"
#include "network/impl/peer_manager_impl.hpp"
#include "network/impl/reputation_repository_impl.hpp"
#include "network/impl/router_libp2p.hpp"
#include "network/impl/state_protocol_observer_impl.hpp"
#include "network/impl/sync_protocol_observer_impl.hpp"
#include "network/impl/synchronizer_impl.hpp"
#include "network/impl/transactions_transmitter_impl.hpp"
#include "network/peer_view.hpp"
#include "offchain/impl/offchain_local_storage.hpp"
#include "offchain/impl/offchain_persistent_storage.hpp"
#include "offchain/impl/offchain_worker_factory_impl.hpp"
#include "offchain/impl/offchain_worker_impl.hpp"
#include "offchain/impl/offchain_worker_pool_impl.hpp"
#include "outcome/outcome.hpp"
#include "parachain/availability/bitfield/store_impl.hpp"
#include "parachain/availability/store/store_impl.hpp"
#include "parachain/pvf/pvf_impl.hpp"
#include "parachain/validator/parachain_observer.hpp"
#include "parachain/validator/parachain_processor.hpp"
#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/core_api_factory_impl.hpp"
#include "runtime/binaryen/instance_environment_factory.hpp"
#include "runtime/binaryen/module/module_factory_impl.hpp"
#include "runtime/common/executor.hpp"
#include "runtime/common/module_repository_impl.hpp"
#include "runtime/common/runtime_instances_pool.hpp"
#include "runtime/common/runtime_upgrade_tracker_impl.hpp"
#include "runtime/common/storage_code_provider.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/runtime_api/impl/account_nonce_api.hpp"
#include "runtime/runtime_api/impl/authority_discovery_api.hpp"
#include "runtime/runtime_api/impl/babe_api.hpp"
#include "runtime/runtime_api/impl/block_builder.hpp"
#include "runtime/runtime_api/impl/core.hpp"
#include "runtime/runtime_api/impl/grandpa_api.hpp"
#include "runtime/runtime_api/impl/metadata.hpp"
#include "runtime/runtime_api/impl/offchain_worker_api.hpp"
#include "runtime/runtime_api/impl/parachain_host.hpp"
#include "runtime/runtime_api/impl/runtime_properties_cache_impl.hpp"
#include "runtime/runtime_api/impl/session_keys_api.hpp"
#include "runtime/runtime_api/impl/tagged_transaction_queue.hpp"
#include "runtime/runtime_api/impl/transaction_payment_api.hpp"
#include "runtime/wavm/compartment_wrapper.hpp"
#include "runtime/wavm/core_api_factory_impl.hpp"
#include "runtime/wavm/instance_environment_factory.hpp"
#include "runtime/wavm/intrinsics/intrinsic_functions.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module_instance.hpp"
#include "runtime/wavm/intrinsics/intrinsic_resolver_impl.hpp"
#include "runtime/wavm/module.hpp"
#include "runtime/wavm/module_cache.hpp"
#include "runtime/wavm/module_factory_impl.hpp"
#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/rocksdb/rocksdb.hpp"
#include "storage/spaces.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/impl/trie_storage_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "telemetry/impl/service_impl.hpp"
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

  template <typename T, typename Fun>
  auto bind_by_lambda(const Fun &fun) {
    return di::bind<T>.to([fun](auto const &injector) {
      static sptr<T> initialized = fun(injector);
      return initialized;
    });
  }

  template <typename Injector>
  auto &get_session_keys(const Injector &injector) {
    static const auto initialized = [&] {
      auto &app_config =
          injector.template create<const application::AppConfiguration &>();
      if (auto dev = app_config.devMnemonicPhrase()) {
        auto &crypto_store = injector.template create<crypto::CryptoStore &>();
        crypto_store.generateEd25519Keypair(crypto::KEY_TYPE_GRAN, *dev)
            .value();
        crypto_store.generateSr25519Keypair(crypto::KEY_TYPE_BABE, *dev)
            .value();
        crypto_store.generateSr25519Keypair(crypto::KEY_TYPE_IMON, *dev)
            .value();
        crypto_store.generateSr25519Keypair(crypto::KEY_TYPE_AUDI, *dev)
            .value();
        crypto_store.generateSr25519Keypair(crypto::KEY_TYPE_ASGN, *dev)
            .value();
        crypto_store.generateSr25519Keypair(crypto::KEY_TYPE_PARA, *dev)
            .value();
      }
      return injector.template create<sptr<crypto::SessionKeys>>();
    }();
    return initialized;
  }

  sptr<api::HttpListenerImpl> get_jrpc_api_http_listener(
      application::AppConfiguration const &config,
      sptr<application::AppStateManager> app_state_manager,
      sptr<api::RpcContext> context,
      api::HttpSession::Configuration http_session_config) {
    static auto initialized =
        std::optional<sptr<api::HttpListenerImpl>>(std::nullopt);
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
        std::optional<sptr<api::WsListenerImpl>>(std::nullopt);
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

  sptr<storage::trie::TrieStorageBackendImpl> get_trie_storage_backend(
      sptr<storage::SpacedStorage> spaced_storage) {
    static auto initialized =
        std::optional<sptr<storage::trie::TrieStorageBackendImpl>>(
            std::nullopt);

    if (initialized) {
      return initialized.value();
    }

    auto storage = spaced_storage->getSpace(storage::Space::kTrieNode);
    auto backend =
        std::make_shared<storage::trie::TrieStorageBackendImpl>(storage);

    initialized.emplace(std::move(backend));
    return initialized.value();
  }

  sptr<storage::SpacedStorage> get_rocks_db(
      application::AppConfiguration const &app_config,
      sptr<application::ChainSpec> chain_spec) {
    static auto initialized =
        std::optional<sptr<storage::SpacedStorage>>(std::nullopt);
    if (initialized) {
      return initialized.value();
    }

    // hack for recovery mode (otherwise - fails due to rocksdb bug)
    bool prevent_destruction = app_config.recoverState().has_value();

    rocksdb::BlockBasedTableOptions table_options;
    table_options.block_cache = rocksdb::NewLRUCache(512 * 1024 * 1024);
    table_options.block_size = 32 * 1024;
    table_options.cache_index_and_filter_blocks = true;
    table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, false));

    auto options = rocksdb::Options{};
    options.create_if_missing = true;
    options.optimize_filters_for_hits = true;
    options.table_factory.reset(
        rocksdb::NewBlockBasedTableFactory(table_options));

    // Setting limit for open rocksdb files to a half of system soft limit
    auto soft_limit = common::getFdLimit();
    if (!soft_limit) {
      exit(EXIT_FAILURE);
    }
    options.max_open_files = soft_limit.value() / 2;

    auto db_res =
        storage::RocksDb::create(app_config.databasePath(chain_spec->id()),
                                 options,
                                 prevent_destruction);
    if (!db_res) {
      auto log = log::createLogger("Injector", "injector");
      log->critical("Can't create RocksDB in {}: {}",
                    fs::absolute(app_config.databasePath(chain_spec->id()),
                                 fs::current_path())
                        .native(),
                    db_res.error());
      exit(EXIT_FAILURE);
    }
    auto &db = db_res.value();

    initialized.emplace(std::move(db));
    return initialized.value();
  }

  std::shared_ptr<application::ChainSpec> get_chain_spec(
      application::AppConfiguration const &config) {
    static auto initialized =
        std::optional<sptr<application::ChainSpec>>(std::nullopt);
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
          chain_spec_res.error());
      exit(EXIT_FAILURE);
    }
    auto &chain_spec = chain_spec_res.value();

    initialized.emplace(std::move(chain_spec));
    return initialized.value();
  }

  sptr<crypto::KeyFileStorage> get_key_file_storage(
      application::AppConfiguration const &config,
      sptr<application::ChainSpec> chain_spec) {
    static std::optional<sptr<crypto::KeyFileStorage>> initialized =
        std::nullopt;
    static std::optional<fs::path> initialized_path = std::nullopt;

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

  sptr<libp2p::protocol::kademlia::Config> get_kademlia_config(
      const application::ChainSpec &chain_spec,
      std::chrono::seconds random_wak_interval) {
    static auto initialized =
        std::optional<sptr<libp2p::protocol::kademlia::Config>>(std::nullopt);
    if (initialized) {
      return initialized.value();
    }

    auto kagome_config = std::make_shared<libp2p::protocol::kademlia::Config>(
        libp2p::protocol::kademlia::Config{
            .protocolId = "/" + chain_spec.protocolId() + "/kad",
            .maxBucketSize = 1000,
            .randomWalk = {.interval = random_wak_interval}});

    initialized.emplace(std::move(kagome_config));
    return initialized.value();
  }

  template <typename Injector>
  sptr<api::ApiServiceImpl> get_jrpc_api_service(const Injector &injector) {
    static auto initialized =
        std::optional<sptr<api::ApiServiceImpl>>(std::nullopt);
    if (initialized) {
      return initialized.value();
    }

    auto app_state_manager =
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
    auto core = injector.template create<sptr<runtime::Core>>();

    auto api_service =
        std::make_shared<api::ApiServiceImpl>(app_state_manager,
                                              thread_pool,
                                              listeners,
                                              server,
                                              processors,
                                              storage_sub_engine,
                                              chain_sub_engine,
                                              ext_sub_engine,
                                              extrinsic_event_key_repo,
                                              block_tree,
                                              trie_storage,
                                              core);

    auto child_state_api =
        injector.template create<std::shared_ptr<api::ChildStateApi>>();
    child_state_api->setApiService(api_service);

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
        std::optional<sptr<blockchain::BlockTree>>(std::nullopt);

    if (initialized) {
      return initialized.value();
    }

    auto header_repo =
        injector.template create<sptr<blockchain::BlockHeaderRepository>>();

    auto storage = injector.template create<sptr<blockchain::BlockStorage>>();

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

    auto changes_tracker = injector.template create<
        std::shared_ptr<storage::changes_trie::ChangesTracker>>();
    auto justification_storage_policy = injector.template create<
        std::shared_ptr<blockchain::JustificationStoragePolicy>>();

    auto block_tree_res = blockchain::BlockTreeImpl::create(
        header_repo,
        std::move(storage),
        std::move(extrinsic_observer),
        std::move(hasher),
        chain_events_engine,
        std::move(ext_events_engine),
        std::move(ext_events_key_repo),
        std::move(changes_tracker),
        std::move(justification_storage_policy));

    if (not block_tree_res.has_value()) {
      common::raise(block_tree_res.error());
    }
    auto &block_tree = block_tree_res.value();

    auto tagged_transaction_queue = injector.template create<
        std::shared_ptr<runtime::TaggedTransactionQueueImpl>>();
    tagged_transaction_queue->setBlockTree(block_tree);

    auto protocol_factory =
        injector.template create<std::shared_ptr<network::ProtocolFactory>>();
    protocol_factory->setBlockTree(block_tree);

    auto runtime_upgrade_tracker =
        injector.template create<sptr<runtime::RuntimeUpgradeTrackerImpl>>();

    runtime_upgrade_tracker->subscribeToBlockchainEvents(chain_events_engine,
                                                         block_tree);

    initialized.emplace(std::move(block_tree));
    return initialized.value();
  }

  template <class Injector>
  sptr<network::PeerManager> get_peer_manager(const Injector &injector) {
    static auto initialized =
        std::optional<sptr<network::PeerManager>>(std::nullopt);
    if (initialized) {
      return initialized.value();
    }

    auto peer_manager = std::make_shared<network::PeerManagerImpl>(
        injector.template create<sptr<application::AppStateManager>>(),
        injector.template create<libp2p::Host &>(),
        injector.template create<sptr<libp2p::protocol::Identify>>(),
        injector.template create<sptr<libp2p::protocol::kademlia::Kademlia>>(),
        injector.template create<sptr<libp2p::basic::Scheduler>>(),
        injector.template create<sptr<network::StreamEngine>>(),
        injector.template create<const application::AppConfiguration &>(),
        injector.template create<sptr<clock::SteadyClock>>(),
        injector.template create<const network::BootstrapNodes &>(),
        injector.template create<const network::OwnPeerInfo &>(),
        injector.template create<sptr<network::Router>>(),
        injector.template create<sptr<storage::SpacedStorage>>(),
        injector.template create<sptr<crypto::Hasher>>(),
        injector.template create<sptr<network::ReputationRepository>>(),
        injector.template create<sptr<network::PeerView>>());

    auto protocol_factory =
        injector.template create<std::shared_ptr<network::ProtocolFactory>>();

    protocol_factory->setPeerManager(peer_manager);

    static auto address_publisher =
        injector.template create<sptr<authority_discovery::AddressPublisher>>();

    initialized.emplace(std::move(peer_manager));
    return initialized.value();
  }

  template <typename Injector>
  sptr<consensus::babe::BlockExecutorImpl> get_block_executor(
      const Injector &injector) {
    static auto initialized =
        std::optional<sptr<consensus::babe::BlockExecutorImpl>>(std::nullopt);
    if (initialized) {
      return initialized.value();
    }

    auto block_executor = std::make_shared<consensus::babe::BlockExecutorImpl>(
        injector.template create<sptr<blockchain::BlockTree>>(),
        injector.template create<sptr<runtime::Core>>(),
        injector.template create<sptr<transaction_pool::TransactionPool>>(),
        injector.template create<sptr<crypto::Hasher>>(),
        injector.template create<sptr<runtime::OffchainWorkerApi>>(),
        injector.template create<uptr<consensus::babe::BlockAppenderBase>>());

    initialized.emplace(std::move(block_executor));
    return initialized.value();
  }

  template <typename Injector>
  sptr<parachain::ParachainObserverImpl> get_parachain_observer_impl(
      const Injector &injector) {
    auto get_instance = [&]() {
      auto instance = std::make_shared<parachain::ParachainObserverImpl>(
          injector.template create<std::shared_ptr<network::PeerManager>>(),
          injector.template create<std::shared_ptr<crypto::Sr25519Provider>>(),
          injector.template create<
              std::shared_ptr<parachain::ParachainProcessorImpl>>());

      auto protocol_factory =
          injector.template create<std::shared_ptr<network::ProtocolFactory>>();

      protocol_factory->setCollactionObserver(instance);
      protocol_factory->setReqCollationObserver(instance);
      return instance;
    };

    static auto instance = get_instance();
    return instance;
  }

  template <typename Injector>
  sptr<parachain::ParachainProcessorImpl> get_parachain_processor_impl(
      const Injector &injector) {
    auto get_instance = [&]() {
      auto &session_keys = get_session_keys(injector);
      auto ptr = std::make_shared<parachain::ParachainProcessorImpl>(
          injector.template create<std::shared_ptr<network::PeerManager>>(),
          injector.template create<std::shared_ptr<crypto::Sr25519Provider>>(),
          injector.template create<std::shared_ptr<network::Router>>(),
          injector
              .template create<std::shared_ptr<::boost::asio::io_context>>(),
          session_keys->getBabeKeyPair(),
          injector.template create<std::shared_ptr<crypto::Hasher>>());

      auto app_state_manager =
          injector
              .template create<std::shared_ptr<application::AppStateManager>>();
      app_state_manager->takeControl(*ptr);
      return ptr;
    };

    static auto instance = get_instance();
    return instance;
  }

  template <typename... Ts>
  auto makeWavmInjector(
      application::AppConfiguration::RuntimeExecutionMethod method,
      Ts &&...args) {
    return di::make_injector(
        di::bind<runtime::wavm::CompartmentWrapper>.template to(
            [](const auto &injector) {
              static auto compartment =
                  std::make_shared<kagome::runtime::wavm::CompartmentWrapper>(
                      "Runtime Compartment");
              return compartment;
            }),
        di::bind<runtime::wavm::IntrinsicModule>.template to(
            [](const auto &injector) {
              static std::shared_ptr<runtime::wavm::IntrinsicModule> module =
                  [&injector]() {
                    auto compartment = injector.template create<
                        sptr<runtime::wavm::CompartmentWrapper>>();
                    runtime::wavm::ModuleParams module_params{};
                    auto module =
                        std::make_unique<runtime::wavm::IntrinsicModule>(
                            compartment, module_params.intrinsicMemoryType);
                    runtime::wavm::registerHostApiMethods(*module);

                    return module;
                  }();
              return module;
            }),
        di::bind<runtime::wavm::IntrinsicModuleInstance>.template to(
            [](const auto &injector) {
              static std::shared_ptr<runtime::wavm::IntrinsicModuleInstance>
                  instance = [&injector]() {
                    auto module = injector.template create<
                        sptr<runtime::wavm::IntrinsicModule>>();
                    return module->instantiate();
                  }();
              return instance;
            }),
        di::bind<runtime::wavm::IntrinsicResolver>.template to<runtime::wavm::IntrinsicResolverImpl>(),
        std::forward<decltype(args)>(args)...);
  }

  template <typename... Ts>
  auto makeBinaryenInjector(
      application::AppConfiguration::RuntimeExecutionMethod method,
      Ts &&...args) {
    return di::make_injector(
        di::bind<runtime::binaryen::RuntimeExternalInterface>.template to(
            [](const auto &injector) {
              static auto rei = [&injector]() {
                auto host_api =
                    injector.template create<sptr<host_api::HostApi>>();
                auto rei = std::make_shared<
                    runtime::binaryen::RuntimeExternalInterface>(host_api);
                auto memory_provider = injector.template create<
                    sptr<runtime::binaryen::BinaryenMemoryProvider>>();
                memory_provider->setExternalInterface(rei);
                return rei;
              }();
              return rei;
            }),
        std::forward<decltype(args)>(args)...);
  }

  template <typename CommonType,
            typename BinaryenType,
            typename WavmType,
            typename Injector>
  auto choose_runtime_implementation(
      Injector const &injector,
      application::AppConfiguration::RuntimeExecutionMethod method) {
    using RuntimeExecutionMethod =
        application::AppConfiguration::RuntimeExecutionMethod;
    static sptr<CommonType> impl = [method, &injector]() {
      switch (method) {
        case RuntimeExecutionMethod::Interpret:
          return std::static_pointer_cast<CommonType>(
              injector.template create<sptr<BinaryenType>>());
        case RuntimeExecutionMethod::Compile:
          return std::static_pointer_cast<CommonType>(
              injector.template create<sptr<WavmType>>());
      }
      throw std::runtime_error("Unknown runtime execution method");
    }();
    return impl;
  }

  template <typename Injector>
  std::shared_ptr<runtime::RuntimeUpgradeTrackerImpl>
  get_runtime_upgrade_tracker(const Injector &injector) {
    static std::shared_ptr<runtime::RuntimeUpgradeTrackerImpl> instance =
        [&injector]() {
          auto header_repo = injector.template create<
              sptr<const blockchain::BlockHeaderRepository>>();
          auto storage =
              injector.template create<sptr<storage::SpacedStorage>>();
          auto substitutes = injector.template create<
              sptr<const primitives::CodeSubstituteBlockIds>>();
          auto block_storage =
              injector.template create<sptr<blockchain::BlockStorage>>();
          auto res = runtime::RuntimeUpgradeTrackerImpl::create(
              std::move(header_repo),
              std::move(storage),
              std::move(substitutes),
              std::move(block_storage));
          if (res.has_error()) {
            throw std::runtime_error(
                "Error creating RuntimeUpgradeTrackerImpl: "
                + res.error().message());
          }
          return std::shared_ptr<runtime::RuntimeUpgradeTrackerImpl>(
              std::move(res.value()));
        }();
    return instance;
  }

  template <typename... Ts>
  auto makeRuntimeInjector(
      application::AppConfiguration::RuntimeExecutionMethod method,
      Ts &&...args) {
    return di::make_injector(
        di::bind<runtime::RuntimeUpgradeTrackerImpl>.template to(
            [](auto const &injector) {
              return get_runtime_upgrade_tracker(injector);
            }),
        di::bind<runtime::RuntimeUpgradeTracker>.template to(
            [](auto const &injector) {
              return get_runtime_upgrade_tracker(injector);
            }),
        makeWavmInjector(method),
        makeBinaryenInjector(method),
        di::bind<runtime::ModuleRepository>.template to<runtime::ModuleRepositoryImpl>(),
        di::bind<runtime::CoreApiFactory>.template to(
            [method](const auto &injector) {
              return choose_runtime_implementation<
                  runtime::CoreApiFactory,
                  runtime::binaryen::CoreApiFactoryImpl,
                  runtime::wavm::CoreApiFactoryImpl>(injector, method);
            }),
        di::bind<runtime::wavm::ModuleFactoryImpl>.template to(
            [](const auto &injector) {
              std::optional<std::shared_ptr<runtime::wavm::ModuleCache>>
                  module_cache_opt;
              auto &app_config =
                  injector
                      .template create<const application::AppConfiguration &>();
              if (app_config.useWavmCache()) {
                module_cache_opt = std::make_shared<runtime::wavm::ModuleCache>(
                    injector.template create<sptr<crypto::Hasher>>(),
                    app_config.runtimeCacheDirPath());
              }
              return std::make_shared<runtime::wavm::ModuleFactoryImpl>(
                  injector.template create<
                      sptr<runtime::wavm::CompartmentWrapper>>(),
                  injector.template create<sptr<runtime::wavm::ModuleParams>>(),
                  injector.template create<
                      sptr<runtime::wavm::InstanceEnvironmentFactory>>(),
                  injector
                      .template create<sptr<runtime::wavm::IntrinsicModule>>(),
                  module_cache_opt,
                  injector.template create<sptr<crypto::Hasher>>());
            }),
        di::bind<runtime::ModuleFactory>.template to(
            [method](const auto &injector) {
              return choose_runtime_implementation<
                  runtime::ModuleFactory,
                  runtime::binaryen::ModuleFactoryImpl,
                  runtime::wavm::ModuleFactoryImpl>(injector, method);
            }),
        di::bind<runtime::RawExecutor>.template to<runtime::Executor>(),
        di::bind<runtime::TaggedTransactionQueue>.template to<runtime::TaggedTransactionQueueImpl>(),
        di::bind<runtime::ParachainHost>.template to<runtime::ParachainHostImpl>(),
        di::bind<runtime::OffchainWorkerApi>.template to<runtime::OffchainWorkerApiImpl>(),
        di::bind<offchain::OffchainWorkerFactory>.template to<offchain::OffchainWorkerFactoryImpl>(),
        di::bind<offchain::OffchainWorker>.template to<offchain::OffchainWorkerImpl>(),
        di::bind<offchain::OffchainWorkerPool>.template to<offchain::OffchainWorkerPoolImpl>(),
        di::bind<offchain::OffchainPersistentStorage>.template to<offchain::OffchainPersistentStorageImpl>(),
        di::bind<offchain::OffchainLocalStorage>.template to<offchain::OffchainLocalStorageImpl>(),
        di::bind<runtime::Metadata>.template to<runtime::MetadataImpl>(),
        di::bind<runtime::GrandpaApi>.template to<runtime::GrandpaApiImpl>(),
        di::bind<runtime::Core>.template to<runtime::CoreImpl>(),
        di::bind<runtime::BabeApi>.template to<runtime::BabeApiImpl>(),
        di::bind<runtime::SessionKeysApi>.template to<runtime::SessionKeysApiImpl>(),
        di::bind<runtime::BlockBuilder>.template to<runtime::BlockBuilderImpl>(),
        di::bind<runtime::TransactionPaymentApi>.template to<runtime::TransactionPaymentApiImpl>(),
        di::bind<runtime::AccountNonceApi>.template to<runtime::AccountNonceApiImpl>(),
        di::bind<runtime::AuthorityDiscoveryApi>.template to<runtime::AuthorityDiscoveryApiImpl>(),
        di::bind<runtime::SingleModuleCache>.template to<runtime::SingleModuleCache>(),
        di::bind<runtime::RuntimePropertiesCache>.template to<runtime::RuntimePropertiesCacheImpl>(),
        std::forward<Ts>(args)...);
  }

  template <typename Injector>
  sptr<primitives::GenesisBlockHeader> get_genesis_block_header(
      const Injector &injector) {
    static auto initialized =
        std::optional<sptr<primitives::GenesisBlockHeader>>(std::nullopt);
    if (initialized) {
      return initialized.value();
    }

    auto block_storage =
        injector.template create<sptr<blockchain::BlockStorage>>();
    auto block_header_repository =
        injector.template create<sptr<blockchain::BlockHeaderRepository>>();

    auto hash_res =
        block_header_repository->getHashByNumber(primitives::BlockNumber(0));
    BOOST_ASSERT(hash_res.has_value());
    auto &hash = hash_res.value();

    auto header_res = block_storage->getBlockHeader(hash);
    BOOST_ASSERT(header_res.has_value());
    auto &header_opt = header_res.value();
    BOOST_ASSERT(header_opt.has_value());

    initialized.emplace(new primitives::GenesisBlockHeader(
        {.header = header_opt.value(), .hash = hash}));

    return initialized.value();
  }

  template <typename... Ts>
  auto makeApplicationInjector(const application::AppConfiguration &config,
                               Ts &&...args) {
    // default values for configurations
    api::RpcThreadPool::Configuration rpc_thread_pool_config{};
    api::HttpSession::Configuration http_config{};
    api::WsSession::Configuration ws_config{};
    transaction_pool::PoolModeratorImpl::Params pool_moderator_config{};
    transaction_pool::TransactionPool::Limits tp_pool_limits{};
    libp2p::protocol::PingConfig ping_config{};
    host_api::OffchainExtensionConfig offchain_ext_config{
        config.isOffchainIndexingEnabled()};

    auto get_state_observer_impl = [](auto const &injector) {
      auto state_observer =
          std::make_shared<network::StateProtocolObserverImpl>(
              injector
                  .template create<sptr<blockchain::BlockHeaderRepository>>(),
              injector.template create<sptr<storage::trie::TrieStorage>>());

      auto protocol_factory =
          injector.template create<std::shared_ptr<network::ProtocolFactory>>();

      protocol_factory->setStateObserver(state_observer);

      return state_observer;
    };

    auto get_sync_observer_impl = [](auto const &injector) {
      auto sync_observer = std::make_shared<network::SyncProtocolObserverImpl>(
          injector.template create<sptr<blockchain::BlockTree>>(),
          injector.template create<sptr<blockchain::BlockHeaderRepository>>());

      auto protocol_factory =
          injector.template create<std::shared_ptr<network::ProtocolFactory>>();

      protocol_factory->setSyncObserver(sync_observer);

      return sync_observer;
    };

    return di::make_injector(
        // bind configs
        useConfig(rpc_thread_pool_config),
        useConfig(http_config),
        useConfig(ws_config),
        useConfig(pool_moderator_config),
        useConfig(tp_pool_limits),
        useConfig(ping_config),
        useConfig(offchain_ext_config),

        // inherit host injector
        libp2p::injector::makeHostInjector(
            libp2p::injector::useSecurityAdaptors<
                libp2p::security::Noise>()[di::override]),

        // inherit kademlia injector
        libp2p::injector::makeKademliaInjector(),
        di::bind<libp2p::protocol::kademlia::Config>.to(
            [random_walk{config.getRandomWalkInterval()}](
                auto const &injector) {
              auto &chain_spec =
                  injector.template create<application::ChainSpec &>();
              return get_kademlia_config(chain_spec, random_walk);
            })[boost::di::override],

        di::bind<application::AppStateManager>.template to<application::AppStateManagerImpl>(),
        di::bind<application::AppConfiguration>.to(config),
        di::bind<primitives::CodeSubstituteBlockIds>.to(
            get_chain_spec(config)->codeSubstitutes()),

        // compose peer keypair
        di::bind<libp2p::crypto::KeyPair>.to([](auto const &injector) {
          auto &app_config =
              injector.template create<const application::AppConfiguration &>();
          auto &crypto_provider =
              injector.template create<const crypto::Ed25519Provider &>();
          auto &crypto_store =
              injector.template create<crypto::CryptoStore &>();
          return injector::get_peer_keypair(
              app_config, crypto_provider, crypto_store);
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
                  std::shared_ptr<api::child_state::ChildStateJrpcProcessor>>(),
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
                  std::shared_ptr<api::payment::PaymentJRpcProcessor>>(),
              injector.template create<
                  std::shared_ptr<api::internal::InternalJrpcProcessor>>()};
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
        di::bind<api::ChildStateApi>.template to<api::ChildStateApiImpl>(),
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
        di::bind<storage::SpacedStorage>.to([](const auto &injector) {
          const application::AppConfiguration &config =
              injector.template create<application::AppConfiguration const &>();
          auto chain_spec =
              injector.template create<sptr<application::ChainSpec>>();
          BOOST_ASSERT(  // since rocksdb is the only possible option now
              config.storageBackend()
              == application::AppConfiguration::StorageBackend::RocksDB);
          return get_rocks_db(config, chain_spec);
        }),
        bind_by_lambda<blockchain::BlockStorage>([](const auto &injector) {
          auto root =
              injector::calculate_genesis_state(
                  injector.template create<const application::ChainSpec &>(),
                  injector.template create<const runtime::ModuleFactory &>(),
                  injector.template create<storage::trie::TrieSerializer &>())
                  .value();
          const auto &hasher = injector.template create<sptr<crypto::Hasher>>();
          const auto &storage =
              injector.template create<sptr<storage::SpacedStorage>>();
          return blockchain::BlockStorageImpl::create(root, storage, hasher)
              .value();
        }),
        di::bind<blockchain::JustificationStoragePolicy>.template to<blockchain::JustificationStoragePolicyImpl>(),
        di::bind<blockchain::BlockTree>.to(
            [](auto const &injector) { return get_block_tree(injector); }),
        di::bind<blockchain::BlockHeaderRepository>.template to<blockchain::BlockHeaderRepositoryImpl>(),
        di::bind<clock::SystemClock>.template to<clock::SystemClockImpl>(),
        di::bind<clock::SteadyClock>.template to<clock::SteadyClockImpl>(),
        di::bind<clock::Timer>.template to<clock::BasicWaitableTimer>(),
        di::bind<network::Synchronizer>.template to<network::SynchronizerImpl>(),
        di::bind<consensus::grandpa::Environment>.template to<consensus::grandpa::EnvironmentImpl>(),
        di::bind<consensus::babe::BlockValidator>.template to<consensus::babe::BabeBlockValidator>(),
        di::bind<crypto::EcdsaProvider>.template to<crypto::EcdsaProviderImpl>(),
        di::bind<crypto::Ed25519Provider>.template to<crypto::Ed25519ProviderImpl>(),
        di::bind<crypto::Hasher>.template to<crypto::HasherImpl>(),
        di::bind<crypto::Sr25519Provider>.template to<crypto::Sr25519ProviderImpl>(),
        di::bind<crypto::VRFProvider>.template to<crypto::VRFProviderImpl>(),
        di::bind<network::StreamEngine>.template to<network::StreamEngine>(),
        di::bind<network::ReputationRepository>.template to<network::ReputationRepositoryImpl>(),
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
        di::bind<host_api::HostApiFactory>.template to<host_api::HostApiFactoryImpl>(),
        makeRuntimeInjector(config.runtimeExecMethod()),
        di::bind<transaction_pool::TransactionPool>.template to<transaction_pool::TransactionPoolImpl>(),
        di::bind<transaction_pool::PoolModerator>.template to<transaction_pool::PoolModeratorImpl>(),
        di::bind<storage::changes_trie::ChangesTracker>.template to<storage::changes_trie::StorageChangesTrackerImpl>(),
        bind_by_lambda<network::StateProtocolObserver>(get_state_observer_impl),
        bind_by_lambda<network::SyncProtocolObserver>(get_sync_observer_impl),
        di::bind<parachain::AvailabilityStore>.template to<parachain::AvailabilityStoreImpl>(),
        di::bind<parachain::BitfieldStore>.template to<parachain::BitfieldStoreImpl>(),
        di::bind<parachain::Pvf>.template to<parachain::PvfImpl>(),
        di::bind<parachain::ParachainObserverImpl>.to([](auto const &injector) {
          return get_parachain_observer_impl(injector);
        }),
        di::bind<parachain::ParachainProcessorImpl>.to(
            [](auto const &injector) {
              return get_parachain_processor_impl(injector);
            }),
        di::bind<storage::trie::TrieStorageBackend>.to(
            [](auto const &injector) {
              auto storage =
                  injector.template create<sptr<storage::SpacedStorage>>();
              return get_trie_storage_backend(storage);
            }),
        bind_by_lambda<storage::trie::TrieStorage>([](auto const &injector) {
          return storage::trie::TrieStorageImpl::createEmpty(
                     injector.template create<
                         sptr<storage::trie::PolkadotTrieFactory>>(),
                     injector.template create<sptr<storage::trie::Codec>>(),
                     injector.template create<
                         sptr<storage::trie::TrieSerializer>>(),
                     injector.template create<
                         sptr<storage::changes_trie::ChangesTracker>>())
              .value();
        }),
        di::bind<storage::trie::PolkadotTrieFactory>.template to<storage::trie::PolkadotTrieFactoryImpl>(),
        di::bind<storage::trie::Codec>.template to<storage::trie::PolkadotCodec>(),
        di::bind<storage::trie::TrieSerializer>.template to<storage::trie::TrieSerializerImpl>(),
        di::bind<runtime::RuntimeCodeProvider>.template to<runtime::StorageCodeProvider>(),
        di::bind<application::ChainSpec>.to([](const auto &injector) {
          const application::AppConfiguration &config =
              injector.template create<application::AppConfiguration const &>();
          return get_chain_spec(config);
        }),
        di::bind<network::ExtrinsicObserver>.to([](const auto &injector) {
          return get_extrinsic_observer_impl(injector);
        }),
        di::bind<consensus::grandpa::GrandpaDigestObserver>.template to<consensus::grandpa::AuthorityManagerImpl>(),
        bind_by_lambda<consensus::grandpa::AuthorityManager>(
            [](auto const &injector) {
              auto auth_manager_impl = injector.template create<
                  sptr<consensus::grandpa::AuthorityManagerImpl>>();
              auto block_tree_impl =
                  injector.template create<sptr<blockchain::BlockTree>>();
              auto justification_storage_policy = injector.template create<
                  sptr<blockchain::JustificationStoragePolicyImpl>>();
              justification_storage_policy->initBlockchainInfo(block_tree_impl);
              return auth_manager_impl;
            }),
        di::bind<network::PeerManager>.to(
            [](auto const &injector) { return get_peer_manager(injector); }),
        di::bind<network::Router>.template to<network::RouterLibp2p>(),
        di::bind<consensus::babe::BlockHeaderAppender>.template to<consensus::babe::BlockHeaderAppenderImpl>(),
        di::bind<consensus::babe::BlockExecutor>.to(
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
        di::bind<consensus::babe::BabeUtil>.template to<consensus::babe::BabeConfigRepositoryImpl>(),
        di::bind<network::BlockAnnounceTransmitter>.template to<network::BlockAnnounceTransmitterImpl>(),
        di::bind<network::GrandpaTransmitter>.template to<network::GrandpaTransmitterImpl>(),
        di::bind<network::TransactionsTransmitter>.template to<network::TransactionsTransmitterImpl>(),
        di::bind<primitives::GenesisBlockHeader>.to([](auto const &injector) {
          return get_genesis_block_header(injector);
        }),
        di::bind<application::mode::RecoveryMode>.to(
            [](auto const &injector) { return get_recovery_mode(injector); }),
        di::bind<telemetry::TelemetryService>.template to<telemetry::TelemetryServiceImpl>(),
        di::bind<consensus::babe::ConsistencyKeeper>.template to<consensus::babe::ConsistencyKeeperImpl>(),
        di::bind<api::InternalApi>.template to<api::InternalApiImpl>(),
        di::bind<consensus::babe::BabeConfigRepository>.template to<consensus::babe::BabeConfigRepositoryImpl>(),
        di::bind<blockchain::DigestTracker>.template to<blockchain::DigestTrackerImpl>(),
        di::bind<consensus::babe::BabeDigestObserver>.template to<consensus::babe::BabeConfigRepositoryImpl>(),

        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }

  template <typename Injector>
  sptr<network::OwnPeerInfo> get_own_peer_info(const Injector &injector) {
    static std::optional<sptr<network::OwnPeerInfo>> initialized{std::nullopt};
    if (initialized) {
      return initialized.value();
    }

    libp2p::crypto::PublicKey public_key;

    const auto &config =
        injector.template create<application::AppConfiguration const &>();

    if (config.roles().flags.authority) {
      auto &crypto_provider =
          injector.template create<const crypto::Ed25519Provider &>();
      auto &crypto_store = injector.template create<crypto::CryptoStore &>();

      auto &local_pair =
          injector::get_peer_keypair(config, crypto_provider, crypto_store);

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
        std::optional<sptr<consensus::babe::Babe>>(std::nullopt);
    if (initialized) {
      return initialized.value();
    }

    auto &session_keys = get_session_keys(injector);

    initialized = std::make_shared<consensus::babe::BabeImpl>(
        injector.template create<const application::AppConfiguration &>(),
        injector.template create<sptr<application::AppStateManager>>(),
        injector.template create<sptr<consensus::babe::BabeLottery>>(),
        injector.template create<sptr<consensus::babe::BabeConfigRepository>>(),
        injector.template create<sptr<authorship::Proposer>>(),
        injector.template create<sptr<blockchain::BlockTree>>(),
        injector.template create<sptr<network::BlockAnnounceTransmitter>>(),
        injector.template create<sptr<crypto::Sr25519Provider>>(),
        session_keys->getBabeKeyPair(),
        injector.template create<sptr<clock::SystemClock>>(),
        injector.template create<sptr<crypto::Hasher>>(),
        injector.template create<uptr<clock::Timer>>(),
        injector.template create<sptr<blockchain::DigestTracker>>(),
        injector.template create<sptr<network::Synchronizer>>(),
        injector.template create<sptr<consensus::babe::BabeUtil>>(),
        injector
            .template create<primitives::events::ChainSubscriptionEnginePtr>(),
        injector.template create<sptr<runtime::OffchainWorkerApi>>(),
        injector.template create<sptr<runtime::Core>>(),
        injector.template create<sptr<consensus::babe::ConsistencyKeeper>>(),
        injector.template create<sptr<storage::trie::TrieStorage>>());

    auto protocol_factory =
        injector.template create<std::shared_ptr<network::ProtocolFactory>>();

    protocol_factory->setBabe(initialized.value());

    return initialized.value();
  }

  template <typename Injector>
  sptr<network::ExtrinsicObserverImpl> get_extrinsic_observer_impl(
      const Injector &injector) {
    static auto initialized =
        std::optional<sptr<network::ExtrinsicObserverImpl>>(std::nullopt);
    if (initialized) {
      return initialized.value();
    }

    initialized = std::make_shared<network::ExtrinsicObserverImpl>(
        injector.template create<sptr<transaction_pool::TransactionPool>>());

    auto protocol_factory =
        injector.template create<std::shared_ptr<network::ProtocolFactory>>();

    protocol_factory->setExtrinsicObserver(initialized.value());

    return initialized.value();
  }

  template <typename Injector>
  sptr<consensus::grandpa::GrandpaImpl> get_grandpa_impl(
      const Injector &injector) {
    static auto initialized =
        std::optional<sptr<consensus::grandpa::GrandpaImpl>>(std::nullopt);
    if (initialized) {
      return initialized.value();
    }

    auto &session_keys = get_session_keys(injector);

    initialized = std::make_shared<consensus::grandpa::GrandpaImpl>(
        injector.template create<sptr<application::AppStateManager>>(),
        injector.template create<sptr<consensus::grandpa::Environment>>(),
        injector.template create<sptr<crypto::Ed25519Provider>>(),
        injector.template create<sptr<runtime::GrandpaApi>>(),
        session_keys->getGranKeyPair(),
        injector.template create<const application::ChainSpec &>(),
        injector.template create<sptr<clock::SteadyClock>>(),
        injector.template create<sptr<libp2p::basic::Scheduler>>(),
        injector.template create<sptr<consensus::grandpa::AuthorityManager>>(),
        injector.template create<sptr<network::Synchronizer>>(),
        injector.template create<sptr<network::PeerManager>>(),
        injector.template create<sptr<blockchain::BlockTree>>(),
        injector.template create<sptr<network::ReputationRepository>>());

    auto protocol_factory =
        injector.template create<std::shared_ptr<network::ProtocolFactory>>();

    protocol_factory->setGrandpaObserver(initialized.value());

    return initialized.value();
  }

  template <typename Injector>
  sptr<application::mode::RecoveryMode> get_recovery_mode(
      const Injector &injector) {
    static auto initialized =
        std::optional<sptr<application::mode::RecoveryMode>>(std::nullopt);
    if (initialized) {
      return initialized.value();
    }

    const auto &app_config =
        injector.template create<const application::AppConfiguration &>();
    auto spaced_storage =
        injector.template create<sptr<storage::SpacedStorage>>();
    auto storage = injector.template create<sptr<blockchain::BlockStorage>>();
    auto header_repo =
        injector.template create<sptr<blockchain::BlockHeaderRepository>>();
    auto trie_storage =
        injector.template create<sptr<const storage::trie::TrieStorage>>();
    auto authority_manager =
        injector.template create<sptr<consensus::grandpa::AuthorityManager>>();
    auto block_tree = injector.template create<sptr<blockchain::BlockTree>>();

    initialized.emplace(new application::mode::RecoveryMode(
        [&app_config,
         spaced_storage = std::move(spaced_storage),
         authority_manager,
         storage = std::move(storage),
         header_repo = std::move(header_repo),
         trie_storage = std::move(trie_storage),
         block_tree = std::move(block_tree)] {
          BOOST_ASSERT(app_config.recoverState().has_value());
          auto res = blockchain::BlockTreeImpl::recover(
              app_config.recoverState().value(),
              storage,
              header_repo,
              trie_storage,
              block_tree);

          auto log = log::createLogger("RecoveryMode", "main");

          spaced_storage->getSpace(storage::Space::kDefault)
              ->remove(storage::kAuthorityManagerStateLookupKey("last"))
              .value();
          if (res.has_error()) {
            SL_ERROR(log, "Recovery mode has failed: {}", res.error());
            log->flush();
            return EXIT_FAILURE;
          }

          return EXIT_SUCCESS;
        }));

    return initialized.value();
  }

  template <typename... Ts>
  auto makeKagomeNodeInjector(const application::AppConfiguration &app_config,
                              Ts &&...args) {
    using namespace boost;  // NOLINT;

    return di::make_injector(
        makeApplicationInjector(app_config),
        // compose peer info
        di::bind<network::OwnPeerInfo>.to(
            [](const auto &injector) { return get_own_peer_info(injector); }),
        di::bind<consensus::babe::Babe>.to(
            [](auto const &injector) { return get_babe(injector); }),
        di::bind<consensus::babe::BabeLottery>.template to<consensus::babe::BabeLotteryImpl>(),
        di::bind<network::BlockAnnounceObserver>.to(
            [](auto const &injector) { return get_babe(injector); }),

        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }

}  // namespace

namespace kagome::injector {

  class KagomeNodeInjectorImpl {
   public:
    using Injector = decltype(makeKagomeNodeInjector(
        std::declval<application::AppConfiguration const &>()));

    explicit KagomeNodeInjectorImpl(Injector injector)
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

  std::shared_ptr<blockchain::BlockStorage>
  KagomeNodeInjector::injectBlockStorage() {
    return pimpl_->injector_.create<sptr<blockchain::BlockStorage>>();
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

  std::shared_ptr<network::StateProtocolObserver>
  KagomeNodeInjector::injectStateObserver() {
    return pimpl_->injector_.create<sptr<network::StateProtocolObserver>>();
  }

  std::shared_ptr<network::SyncProtocolObserver>
  KagomeNodeInjector::injectSyncObserver() {
    return pimpl_->injector_.create<sptr<network::SyncProtocolObserver>>();
  }

  std::shared_ptr<parachain::ParachainObserverImpl>
  KagomeNodeInjector::injectParachainObserver() {
    return pimpl_->injector_.create<sptr<parachain::ParachainObserverImpl>>();
  }

  std::shared_ptr<parachain::ParachainProcessorImpl>
  KagomeNodeInjector::injectParachainProcessor() {
    return pimpl_->injector_.create<sptr<parachain::ParachainProcessorImpl>>();
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

  std::shared_ptr<storage::trie::TrieStorage>
  KagomeNodeInjector::injectTrieStorage() {
    return pimpl_->injector_.create<sptr<storage::trie::TrieStorage>>();
  }

  std::shared_ptr<metrics::MetricsWatcher>
  KagomeNodeInjector::injectMetricsWatcher() {
    return pimpl_->injector_.create<sptr<metrics::MetricsWatcher>>();
  }

  std::shared_ptr<telemetry::TelemetryService>
  KagomeNodeInjector::injectTelemetryService() {
    return pimpl_->injector_.create<sptr<telemetry::TelemetryService>>();
  }

  std::shared_ptr<application::mode::PrintChainInfoMode>
  KagomeNodeInjector::injectPrintChainInfoMode() {
    return pimpl_->injector_
        .create<sptr<application::mode::PrintChainInfoMode>>();
  }

  std::shared_ptr<application::mode::RecoveryMode>
  KagomeNodeInjector::injectRecoveryMode() {
    return pimpl_->injector_.create<sptr<application::mode::RecoveryMode>>();
  }

  std::shared_ptr<blockchain::BlockTree> KagomeNodeInjector::injectBlockTree() {
    return pimpl_->injector_.create<sptr<blockchain::BlockTree>>();
  }

  std::shared_ptr<runtime::Executor> KagomeNodeInjector::injectExecutor() {
    return pimpl_->injector_.create<sptr<runtime::Executor>>();
  }

  std::shared_ptr<storage::SpacedStorage> KagomeNodeInjector::injectStorage() {
    return pimpl_->injector_.create<sptr<storage::SpacedStorage>>();
  }

}  // namespace kagome::injector
