/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "injector/application_injector.hpp"

#define BOOST_DI_CFG_DIAGNOSTICS_LEVEL 2
#define BOOST_DI_CFG_CTOR_LIMIT_SIZE \
  32  // TODO(Harrm): #2104 check how it influences on compilation time

#include <rocksdb/filter_policy.h>
#include <rocksdb/table.h>
#include <boost/di.hpp>
#include <boost/di/extension/scopes/shared.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/injector/kademlia_injector.hpp>
#include <libp2p/log/configurator.hpp>
#include <libp2p/protocol/ping/ping.hpp>
#include <libp2p/protocol/ping/ping_config.hpp>

#undef U64  // comes from OpenSSL and messes with WAVM

#include "api/service/author/author_jrpc_processor.hpp"
#include "api/service/author/impl/author_api_impl.hpp"
#include "api/service/beefy/rpc.hpp"
#include "api/service/chain/chain_jrpc_processor.hpp"
#include "api/service/chain/impl/chain_api_impl.hpp"
#include "api/service/child_state/child_state_jrpc_processor.hpp"
#include "api/service/child_state/impl/child_state_api_impl.hpp"
#include "api/service/impl/api_service_impl.hpp"
#include "api/service/impl/rpc_thread_pool.hpp"
#include "api/service/internal/impl/internal_api_impl.hpp"
#include "api/service/internal/internal_jrpc_processor.hpp"
#include "api/service/mmr/rpc.hpp"
#include "api/service/payment/impl/payment_api_impl.hpp"
#include "api/service/payment/payment_jrpc_processor.hpp"
#include "api/service/rpc/impl/rpc_api_impl.hpp"
#include "api/service/rpc/rpc_jrpc_processor.hpp"
#include "api/service/state/impl/state_api_impl.hpp"
#include "api/service/state/state_jrpc_processor.hpp"
#include "api/service/system/impl/system_api_impl.hpp"
#include "api/service/system/system_jrpc_processor.hpp"
#include "api/transport/impl/ws/ws_listener_impl.hpp"
#include "api/transport/impl/ws/ws_session.hpp"
#include "application/app_configuration.hpp"
#include "application/impl/app_state_manager_impl.hpp"
#include "application/impl/chain_spec_impl.hpp"
#include "application/modes/precompile_wasm.hpp"
#include "application/modes/print_chain_info_mode.hpp"
#include "application/modes/recovery_mode.hpp"
#include "authority_discovery/publisher/address_publisher.hpp"
#include "authority_discovery/query/query_impl.hpp"
#include "authorship/impl/block_builder_factory_impl.hpp"
#include "authorship/impl/block_builder_impl.hpp"
#include "authorship/impl/proposer_impl.hpp"
#include "benchmark/block_execution_benchmark.hpp"
#include "blockchain/impl/block_header_repository_impl.hpp"
#include "blockchain/impl/block_storage_impl.hpp"
#include "blockchain/impl/block_tree_impl.hpp"
#include "blockchain/impl/justification_storage_policy.hpp"
#include "clock/impl/basic_waitable_timer.hpp"
#include "clock/impl/clock_impl.hpp"
#include "common/fd_limit.hpp"
#include "common/main_thread_pool.hpp"
#include "common/outcome_throw.hpp"
#include "common/worker_thread_pool.hpp"
#include "consensus/babe/impl/babe.hpp"
#include "consensus/babe/impl/babe_block_validator_impl.hpp"
#include "consensus/babe/impl/babe_config_repository_impl.hpp"
#include "consensus/babe/impl/babe_lottery_impl.hpp"
#include "consensus/beefy/impl/beefy_impl.hpp"
#include "consensus/beefy/impl/beefy_thread_pool.hpp"
#include "consensus/finality_consensus.hpp"
#include "consensus/grandpa/impl/authority_manager_impl.hpp"
#include "consensus/grandpa/impl/environment_impl.hpp"
#include "consensus/grandpa/impl/grandpa_impl.hpp"
#include "consensus/grandpa/impl/grandpa_thread_pool.hpp"
#include "consensus/grandpa/impl/verified_justification_queue.hpp"
#include "consensus/production_consensus.hpp"
#include "consensus/timeline/impl/block_appender_base.hpp"
#include "consensus/timeline/impl/block_executor_impl.hpp"
#include "consensus/timeline/impl/block_header_appender_impl.hpp"
#include "consensus/timeline/impl/consensus_selector_impl.hpp"
#include "consensus/timeline/impl/slots_util_impl.hpp"
#include "consensus/timeline/impl/timeline_impl.hpp"
#include "crypto/bandersnatch/bandersnatch_provider_impl.hpp"
#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/ecdsa/ecdsa_provider_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/elliptic_curves/elliptic_curves_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/key_store.hpp"
#include "crypto/key_store/key_store_impl.hpp"
#include "crypto/key_store/session_keys.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "crypto/vrf/vrf_provider_impl.hpp"
#include "dispute_coordinator/impl/dispute_coordinator_impl.hpp"
#include "dispute_coordinator/impl/dispute_thread_pool.hpp"
#include "dispute_coordinator/impl/storage_impl.hpp"
#include "host_api/impl/host_api_factory_impl.hpp"
#include "injector/bind_by_lambda.hpp"
#include "injector/calculate_genesis_state.hpp"
#include "injector/get_peer_keypair.hpp"
#include "injector/idle_trie_pruner.hpp"
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
#include "network/impl/protocols/beefy_justification_protocol.hpp"
#include "network/impl/protocols/beefy_protocol_impl.hpp"
#include "network/impl/protocols/fetch_attested_candidate.hpp"
#include "network/impl/protocols/grandpa_protocol.hpp"
#include "network/impl/protocols/light.hpp"
#include "network/impl/protocols/parachain_protocols.hpp"
#include "network/impl/protocols/protocol_fetch_available_data.hpp"
#include "network/impl/protocols/protocol_fetch_chunk.hpp"
#include "network/impl/protocols/protocol_fetch_chunk_obsolete.hpp"
#include "network/impl/protocols/protocol_req_collation.hpp"
#include "network/impl/protocols/protocol_req_pov.hpp"
#include "network/impl/protocols/send_dispute_protocol.hpp"
#include "network/impl/protocols/state_protocol_impl.hpp"
#include "network/impl/protocols/sync_protocol_impl.hpp"
#include "network/impl/reputation_repository_impl.hpp"
#include "network/impl/router_libp2p.hpp"
#include "network/impl/state_protocol_observer_impl.hpp"
#include "network/impl/sync_protocol_observer_impl.hpp"
#include "network/impl/synchronizer_impl.hpp"
#include "network/impl/transactions_transmitter_impl.hpp"
#include "network/kademlia_random_walk.hpp"
#include "network/warp/cache.hpp"
#include "network/warp/protocol.hpp"
#include "network/warp/sync.hpp"
#include "offchain/impl/offchain_local_storage.hpp"
#include "offchain/impl/offchain_persistent_storage.hpp"
#include "offchain/impl/offchain_worker_factory_impl.hpp"
#include "offchain/impl/offchain_worker_impl.hpp"
#include "offchain/impl/offchain_worker_pool_impl.hpp"
#include "offchain/impl/runner.hpp"
#include "outcome/outcome.hpp"
#include "parachain/approval/approval_distribution.hpp"
#include "parachain/approval/approval_thread_pool.hpp"
#include "parachain/availability/bitfield/store_impl.hpp"
#include "parachain/availability/fetch/fetch_impl.hpp"
#include "parachain/availability/recovery/recovery_impl.hpp"
#include "parachain/availability/store/store_impl.hpp"
#include "parachain/backing/store_impl.hpp"
#include "parachain/pvf/module_precompiler.hpp"
#include "parachain/pvf/pool.hpp"
#include "parachain/pvf/pvf_impl.hpp"
#include "parachain/pvf/pvf_thread_pool.hpp"
#include "parachain/pvf/workers.hpp"
#include "parachain/validator/impl/parachain_observer_impl.hpp"
#include "parachain/validator/parachain_processor.hpp"
#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/instance_environment_factory.hpp"
#include "runtime/binaryen/module/module_factory_impl.hpp"
#include "runtime/common/core_api_factory_impl.hpp"
#include "runtime/common/module_repository_impl.hpp"
#include "runtime/common/runtime_instances_pool.hpp"
#include "runtime/common/runtime_properties_cache_impl.hpp"
#include "runtime/common/runtime_upgrade_tracker_impl.hpp"
#include "runtime/common/storage_code_provider.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/executor.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/runtime_api/impl/account_nonce_api.hpp"
#include "runtime/runtime_api/impl/authority_discovery_api.hpp"
#include "runtime/runtime_api/impl/babe_api.hpp"
#include "runtime/runtime_api/impl/beefy.hpp"
#include "runtime/runtime_api/impl/block_builder.hpp"
#include "runtime/runtime_api/impl/core.hpp"
#include "runtime/runtime_api/impl/grandpa_api.hpp"
#include "runtime/runtime_api/impl/metadata.hpp"
#include "runtime/runtime_api/impl/mmr.hpp"
#include "runtime/runtime_api/impl/offchain_worker_api.hpp"
#include "runtime/runtime_api/impl/parachain_host.hpp"
#include "runtime/runtime_api/impl/session_keys_api.hpp"
#include "runtime/runtime_api/impl/tagged_transaction_queue.hpp"
#include "runtime/runtime_api/impl/transaction_payment_api.hpp"
#include "runtime/wabt/instrument.hpp"
#include "runtime/wasm_compiler_definitions.hpp"  // this header-file is generated

#if KAGOME_WASM_COMPILER_WASM_EDGE == 1

#include "runtime/wasm_edge/module_factory_impl.hpp"

#endif

#if KAGOME_WASM_COMPILER_WAVM == 1

#include "runtime/wavm/compartment_wrapper.hpp"
#include "runtime/wavm/instance_environment_factory.hpp"
#include "runtime/wavm/intrinsics/intrinsic_functions.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module_instance.hpp"
#include "runtime/wavm/intrinsics/intrinsic_resolver_impl.hpp"
#include "runtime/wavm/module.hpp"
#include "runtime/wavm/module_factory_impl.hpp"

#endif

#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"
#include "storage/rocksdb/rocksdb.hpp"
#include "storage/spaces.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/impl/trie_storage_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "storage/trie_pruner/impl/trie_pruner_impl.hpp"
#include "telemetry/impl/service_impl.hpp"
#include "telemetry/impl/telemetry_thread_pool.hpp"
#include "transaction_pool/impl/pool_moderator_impl.hpp"
#include "transaction_pool/impl/transaction_pool_impl.hpp"

namespace {
  template <class T>
  using sptr = std::shared_ptr<T>;

  template <class T>
  using uptr = std::unique_ptr<T>;

  namespace di = boost::di;
  namespace fs = kagome::filesystem;
  using namespace kagome;  // NOLINT

  template <typename C>
  auto useConfig(C c) {
    return boost::di::bind<std::decay_t<C>>().to(
        std::move(c))[boost::di::override];
  }

  using injector::bind_by_lambda;

  sptr<storage::trie::TrieStorageBackendImpl> get_trie_storage_backend(
      sptr<storage::SpacedStorage> spaced_storage) {
    auto backend = std::make_shared<storage::trie::TrieStorageBackendImpl>(
        std::move(spaced_storage));

    return backend;
  }

  sptr<storage::SpacedStorage> get_rocks_db(
      const application::AppConfiguration &app_config,
      const sptr<application::ChainSpec> &chain_spec) {
    // hack for recovery mode (otherwise - fails due to rocksdb bug)
    bool prevent_destruction = app_config.recoverState().has_value();

    auto options = rocksdb::Options{};
    options.create_if_missing = true;
    options.optimize_filters_for_hits = true;
    options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(
        storage::RocksDb::tableOptionsConfiguration()));

    // Setting limit for open rocksdb files to a half of system soft limit
    auto soft_limit = common::getFdLimit();
    if (!soft_limit) {
      exit(EXIT_FAILURE);
    }
    // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
    options.max_open_files = soft_limit.value() / 2;

    auto db_res =
        storage::RocksDb::create(app_config.databasePath(chain_spec->id()),
                                 options,
                                 app_config.dbCacheSize(),
                                 prevent_destruction);
    if (!db_res) {
      auto log = log::createLogger("Injector", "injector");
      log->critical(
          "Can't create RocksDB in {}: {}",
          fs::absolute(app_config.databasePath(chain_spec->id())).native(),
          db_res.error());
      exit(EXIT_FAILURE);
    }
    auto db = std::move(db_res.value());

    return db;
  }

  std::shared_ptr<application::ChainSpec> get_chain_spec(
      const application::AppConfiguration &config) {
    const auto &chainspec_path = config.chainSpecPath();

    auto chain_spec_res =
        application::ChainSpecImpl::loadFrom(chainspec_path.native());
    if (not chain_spec_res.has_value()) {
      auto log = log::createLogger("Injector", "injector");
      log->critical("Can't load chain spec from {}: {}",
                    fs::absolute(chainspec_path.native()).native(),
                    chain_spec_res.error());
      exit(EXIT_FAILURE);
    }
    auto &chain_spec = chain_spec_res.value();

    return std::move(chain_spec);
  }

  sptr<crypto::KeyFileStorage> get_key_file_storage(
      const application::AppConfiguration &config,
      const sptr<application::ChainSpec> &chain_spec) {
    auto path = config.keystorePath(chain_spec->id());
    auto key_file_storage_res = crypto::KeyFileStorage::createAt(path);
    if (not key_file_storage_res) {
      common::raise(key_file_storage_res.error());
    }

    return std::move(key_file_storage_res.value());
  }

  sptr<libp2p::protocol::kademlia::Config> get_kademlia_config(
      const blockchain::GenesisBlockHash &genesis,
      const application::ChainSpec &chain_spec,
      std::chrono::seconds random_wak_interval) {
    libp2p::protocol::kademlia::Config kademlia_config;
    kademlia_config.protocols =
        network::make_protocols("/{}/kad", genesis, chain_spec);
    kademlia_config.maxBucketSize = 1000;
    kademlia_config.randomWalk.enabled = false;
    kademlia_config.valueLookupsQuorum = 4;

    return std::make_shared<libp2p::protocol::kademlia::Config>(
        std::move(kademlia_config));
  }

  sptr<libp2p::protocol::IdentifyConfig> get_identify_config() {
    libp2p::protocol::IdentifyConfig identify_config;
    identify_config.protocols = {"/ipfs/id/1.0.0", "/substrate/1.0"};
    return std::make_shared<libp2p::protocol::IdentifyConfig>(
        std::move(identify_config));
  }

  template <typename Injector>
  sptr<blockchain::BlockTree> get_block_tree(const Injector &injector) {
    auto chain_events_engine =
        injector
            .template create<primitives::events::ChainSubscriptionEnginePtr>();

    // clang-format off
    auto block_tree_res = blockchain::BlockTreeImpl::create(
        injector.template create<const application::AppConfiguration &>(),
        injector.template create<sptr<blockchain::BlockHeaderRepository>>(),
        injector.template create<sptr<blockchain::BlockStorage>>(),
        injector.template create<sptr<network::ExtrinsicObserver>>(),
        injector.template create<sptr<crypto::Hasher>>(),
        chain_events_engine,
        injector.template create<primitives::events::ExtrinsicSubscriptionEnginePtr>(),
        injector.template create<std::shared_ptr<subscription::ExtrinsicEventKeyRepository>>(),
        injector.template create<std::shared_ptr<blockchain::JustificationStoragePolicy>>(),
        injector.template create<sptr<storage::trie_pruner::TriePruner>>(),
        injector.template create<common::MainThreadPool &>());
    // clang-format on

    if (not block_tree_res.has_value()) {
      common::raise(block_tree_res.error());
    }
    auto &block_tree = block_tree_res.value();

    auto runtime_upgrade_tracker =
        injector.template create<sptr<runtime::RuntimeUpgradeTrackerImpl>>();

    runtime_upgrade_tracker->subscribeToBlockchainEvents(chain_events_engine,
                                                         block_tree);

    return block_tree;
  }

  template <typename... Ts>
  auto makeWavmInjector(Ts &&...args) {
    return di::make_injector(
#if KAGOME_WASM_COMPILER_WAVM == 1
        bind_by_lambda<runtime::wavm::CompartmentWrapper>([](const auto
                                                                 &injector) {
          return std::make_shared<kagome::runtime::wavm::CompartmentWrapper>(
              "Runtime Compartment");
        }),
        bind_by_lambda<runtime::wavm::IntrinsicModule>(
            [](const auto &injector) {
              auto compartment = injector.template create<
                  sptr<runtime::wavm::CompartmentWrapper>>();
              runtime::wavm::ModuleParams module_params{};
              auto module = std::make_unique<runtime::wavm::IntrinsicModule>(
                  compartment, module_params.intrinsicMemoryType);
              runtime::wavm::registerHostApiMethods(*module);
              return module;
            }),
        bind_by_lambda<runtime::wavm::IntrinsicModuleInstance>(
            [](const auto &injector) {
              auto module =
                  injector
                      .template create<sptr<runtime::wavm::IntrinsicModule>>();
              return module->instantiate();
            }),
        di::bind<runtime::wavm::IntrinsicResolver>.template to<runtime::wavm::IntrinsicResolverImpl>(),
#endif
        std::forward<decltype(args)>(args)...);
  }

  template <typename... Ts>
  auto makeBinaryenInjector(Ts &&...args) {
    return di::make_injector(
        bind_by_lambda<runtime::binaryen::RuntimeExternalInterface>(
            [](const auto &injector) {
              auto host_api =
                  injector.template create<sptr<host_api::HostApi>>();
              auto rei =
                  std::make_shared<runtime::binaryen::RuntimeExternalInterface>(
                      host_api);
              auto memory_provider = injector.template create<
                  sptr<runtime::binaryen::BinaryenMemoryProvider>>();
              memory_provider->setExternalInterface(rei);
              return rei;
            }),
        std::forward<decltype(args)>(args)...);
  }

  template <typename... Ts>
  auto makeWasmEdgeInjector(Ts &&...args) {
    return di::make_injector(
#if KAGOME_WASM_COMPILER_WASM_EDGE == 1
        di::bind<runtime::ModuleFactory>.template to<runtime::wasm_edge::ModuleFactoryImpl>(),
#endif
        std::forward<decltype(args)>(args)...);
  }

  template <typename CommonType,
            typename InterpretedType,
            typename CompiledType,
            typename Injector>
  auto choose_runtime_implementation(
      const Injector &injector,
      application::AppConfiguration::RuntimeExecutionMethod method,
      application::AppConfiguration::RuntimeInterpreter interpreter) {
    using RuntimeExecutionMethod =
        application::AppConfiguration::RuntimeExecutionMethod;
    using RuntimeInterpreter =
        application::AppConfiguration::RuntimeInterpreter;
    switch (method) {
      case RuntimeExecutionMethod::Interpret:
        switch (interpreter) {
          case RuntimeInterpreter::Binaryen:
            return std::static_pointer_cast<CommonType>(
                injector.template create<sptr<InterpretedType>>());
          case RuntimeInterpreter::WasmEdge:
            return std::static_pointer_cast<CommonType>(
                injector.template create<sptr<CompiledType>>());
        }
        BOOST_UNREACHABLE_RETURN({});
      case RuntimeExecutionMethod::Compile:
        return std::static_pointer_cast<CommonType>(
            injector.template create<sptr<CompiledType>>());
    }
    throw std::runtime_error("Unknown runtime execution method");
  }

  template <typename Injector>
  std::shared_ptr<runtime::RuntimeUpgradeTrackerImpl>
  get_runtime_upgrade_tracker(const Injector &injector) {
    auto header_repo =
        injector
            .template create<sptr<const blockchain::BlockHeaderRepository>>();
    auto storage = injector.template create<sptr<storage::SpacedStorage>>();
    auto substitutes =
        injector
            .template create<sptr<const primitives::CodeSubstituteBlockIds>>();
    auto block_storage =
        injector.template create<sptr<blockchain::BlockStorage>>();
    auto res =
        runtime::RuntimeUpgradeTrackerImpl::create(std::move(header_repo),
                                                   std::move(storage),
                                                   std::move(substitutes),
                                                   std::move(block_storage));
    return std::shared_ptr<runtime::RuntimeUpgradeTrackerImpl>(
        std::move(res.value()));
  }

#if KAGOME_WASM_COMPILER_WAVM == 1

  using ChosenModuleFactoryImpl = runtime::wavm::ModuleFactoryImpl;

#elif KAGOME_WASM_COMPILER_WASM_EDGE == 1

  using ChosenModuleFactoryImpl = runtime::wasm_edge::ModuleFactoryImpl;

#endif

  template <typename... Ts>
  auto makeRuntimeInjector(
      application::AppConfiguration::RuntimeExecutionMethod method,
      application::AppConfiguration::RuntimeInterpreter interpreter,
      Ts &&...args) {
    return di::make_injector(
        bind_by_lambda<runtime::RuntimeUpgradeTrackerImpl>(
            [](const auto &injector) {
              return get_runtime_upgrade_tracker(injector);
            }),
        bind_by_lambda<runtime::RuntimeUpgradeTracker>(
            [](const auto &injector) {
              return injector
                  .template create<sptr<runtime::RuntimeUpgradeTrackerImpl>>();
            }),
        makeBinaryenInjector(),
        makeWavmInjector(),
        di::bind<runtime::RuntimeInstancesPool>.template to<runtime::RuntimeInstancesPoolImpl>(),
        di::bind<runtime::ModuleRepository>.template to<runtime::ModuleRepositoryImpl>(),
        di::bind<runtime::CoreApiFactory>.template to<runtime::CoreApiFactoryImpl>(),
        bind_by_lambda<runtime::ModuleFactory>(
            [method, interpreter](
                const auto &injector) -> sptr<runtime::ModuleFactory> {
              return choose_runtime_implementation<
                  runtime::ModuleFactory,
                  runtime::binaryen::ModuleFactoryImpl,
                  ChosenModuleFactoryImpl>(injector, method, interpreter);
            }),
        bind_by_lambda<runtime::Executor>([](const auto &injector)
                                              -> sptr<runtime::Executor> {
          auto ctx_factory =
              injector.template create<sptr<runtime::RuntimeContextFactory>>();
          auto cache =
              injector.template create<sptr<runtime::RuntimePropertiesCache>>();
          return std::make_shared<runtime::Executor>(ctx_factory, cache);
        }),
        di::bind<runtime::TaggedTransactionQueue>.template to<runtime::TaggedTransactionQueueImpl>(),
        di::bind<runtime::ParachainHost>.template to<runtime::ParachainHostImpl>(),
        di::bind<runtime::OffchainWorkerApi>.template to<runtime::OffchainWorkerApiImpl>(),
        di::bind<offchain::OffchainWorkerFactory>.template to<offchain::OffchainWorkerFactoryImpl>(),
        di::bind<offchain::OffchainWorker>.template to<offchain::OffchainWorkerImpl>(),
        di::bind<offchain::OffchainWorkerPool>.template to<offchain::OffchainWorkerPoolImpl>(),
        di::bind<offchain::OffchainPersistentStorage>.template to<offchain::OffchainPersistentStorageImpl>(),
        di::bind<offchain::OffchainLocalStorage>.template to<offchain::OffchainLocalStorageImpl>(),
        di::bind<runtime::Metadata>.template to<runtime::MetadataImpl>(),
        di::bind<runtime::MmrApi>.template to<runtime::MmrApiImpl>(),
        di::bind<runtime::GrandpaApi>.template to<runtime::GrandpaApiImpl>(),
        di::bind<runtime::BeefyApi>.template to<runtime::BeefyApiImpl>(),
        di::bind<runtime::Core>.template to<runtime::CoreImpl>(),
        di::bind<runtime::BabeApi>.template to<runtime::BabeApiImpl>(),
        di::bind<runtime::SessionKeysApi>.template to<runtime::SessionKeysApiImpl>(),
        di::bind<runtime::BlockBuilder>.template to<runtime::BlockBuilderImpl>(),
        di::bind<runtime::TransactionPaymentApi>.template to<runtime::TransactionPaymentApiImpl>(),
        di::bind<runtime::AccountNonceApi>.template to<runtime::AccountNonceApiImpl>(),
        di::bind<runtime::AuthorityDiscoveryApi>.template to<runtime::AuthorityDiscoveryApiImpl>(),
        di::bind<runtime::RuntimePropertiesCache>.template to<runtime::RuntimePropertiesCacheImpl>(),
        std::forward<Ts>(args)...);
  }

  template <typename Injector>
  sptr<primitives::GenesisBlockHeader> get_genesis_block_header(
      const Injector &injector) {
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

    return std::make_shared<primitives::GenesisBlockHeader>(
        primitives::GenesisBlockHeader{*header_opt, hash});
  }

  template <typename... Ts>
  auto makeApplicationInjector(sptr<application::AppConfiguration> config,
                               Ts &&...args) {
    // default values for configurations
    api::WsSession::Configuration ws_config{};
    transaction_pool::PoolModeratorImpl::Params pool_moderator_config{};
    transaction_pool::TransactionPool::Limits tp_pool_limits{};
    libp2p::protocol::PingConfig ping_config{};
    host_api::OffchainExtensionConfig offchain_ext_config{
        config->isOffchainIndexingEnabled()};
    parachain::PvfImpl::Config pvf_config{
        .precompile_modules = config->shouldPrecompileParachainModules(),
        .precompile_threads_num = config->parachainPrecompilationThreadNum(),
    };
#if KAGOME_WASM_COMPILER_WASM_EDGE == 1
    runtime::wasm_edge::ModuleFactoryImpl::Config wasmedge_config{
        config->runtimeExecMethod()
                == application::AppConfiguration::RuntimeExecutionMethod::
                    Compile
            ? runtime::wasm_edge::ModuleFactoryImpl::ExecType::Compiled
            : runtime::wasm_edge::ModuleFactoryImpl::ExecType::Interpreted,
    };
#endif

    // clang-format off
    return di::make_injector(
            // bind configs
            useConfig(ws_config),
            useConfig(pool_moderator_config),
            useConfig(tp_pool_limits),
            useConfig(ping_config),
            useConfig(offchain_ext_config),
            useConfig(pvf_config),
#if KAGOME_WASM_COMPILER_WASM_EDGE == 1
            useConfig(wasmedge_config),
#endif
            di::bind<crypto::KeyStore::Config>.to([](const auto &injector) {
              const auto &config =
                  injector
                      .template create<application::AppConfiguration const &>();
              auto chain_spec =
                  injector.template create<sptr<application::ChainSpec>>();
              return crypto::KeyStore::Config{config.keystorePath(chain_spec->id())};
            }),

            // inherit host injector
            libp2p::injector::makeHostInjector(
                libp2p::injector::useWssPem(config->nodeWssPem()),
                libp2p::injector::useSecurityAdaptors<
                    libp2p::security::Noise>()[di::override]),

            // inherit kademlia injector
            libp2p::injector::makeKademliaInjector(),
            bind_by_lambda<libp2p::protocol::kademlia::Config>(
                [random_walk{config->getRandomWalkInterval()}](
                    const auto &injector) {
                  return get_kademlia_config(
                    injector.template create<const blockchain::GenesisBlockHash &>(),
                    injector.template create<const application::ChainSpec &>(), random_walk);
                })[boost::di::override],

            di::bind<application::AppStateManager>.template to<application::AppStateManagerImpl>(),
            di::bind<application::AppConfiguration>.to(config),
            bind_by_lambda<primitives::CodeSubstituteBlockIds>(
                [](auto &&injector) {
                  return std::const_pointer_cast<
                      primitives::CodeSubstituteBlockIds>(
                      injector.template create<const application::ChainSpec &>()
                          .codeSubstitutes());
                }),

            // compose peer keypair
            bind_by_lambda<libp2p::crypto::KeyPair>([](const auto &injector) {
              auto &app_config =
                  injector
                      .template create<const application::AppConfiguration &>();
              auto &chain =
                  injector.template create<const application::ChainSpec &>();
              auto &crypto_provider =
                  injector.template create<const crypto::Ed25519Provider &>();
              auto &csprng = injector.template create<crypto::CSPRNG &>();
              auto &crypto_store =
                  injector.template create<crypto::KeyStore &>();
              return injector::get_peer_keypair(
                  app_config, chain, crypto_provider, csprng, crypto_store);
            })[boost::di::override],

            di::bind<api::Listener *[]>()  // NOLINT
                .template to<api::WsListenerImpl>(),
            di::bind<api::JRpcProcessor *[]>()  // NOLINT
                .template to<api::child_state::ChildStateJrpcProcessor,
                             api::BeefyRpc,
                             api::MmrRpc,
                             api::state::StateJrpcProcessor,
                             api::author::AuthorJRpcProcessor,
                             api::chain::ChainJrpcProcessor,
                             api::system::SystemJrpcProcessor,
                             api::rpc::RpcJRpcProcessor,
                             api::payment::PaymentJRpcProcessor,
                             api::internal::InternalJrpcProcessor>(),

            // starting metrics interfaces
            di::bind<metrics::Handler>.template to<metrics::PrometheusHandler>(),
            di::bind<metrics::Exposer>.template to<metrics::ExposerImpl>(),
            di::bind<metrics::Exposer::Configuration>.to([](const auto
                                                                &injector) {
              return metrics::Exposer::Configuration{
                  injector
                      .template create<application::AppConfiguration const &>()
                      .openmetricsHttpEndpoint()};
            }),
            // hardfix for Mac clang
            di::bind<metrics::Session::Configuration>.to(
                [](const auto &injector) {
                  return metrics::Session::Configuration{};
                }),
            // ending metrics interfaces
            di::bind<api::AuthorApi>.template to<api::AuthorApiImpl>(),
            di::bind<network::Roles>.to(config->roles()),
            di::bind<api::ChainApi>.template to<api::ChainApiImpl>(),
            di::bind<api::ChildStateApi>.template to<api::ChildStateApiImpl>(),
            di::bind<api::StateApi>.template to<api::StateApiImpl>(),
            di::bind<api::SystemApi>.template to<api::SystemApiImpl>(),
            di::bind<api::RpcApi>.template to<api::RpcApiImpl>(),
            di::bind<api::PaymentApi>.template to<api::PaymentApiImpl>(),
            di::bind<api::ApiService>.template to<api::ApiServiceImpl>(),
            di::bind<api::JRpcServer>.template to<api::JRpcServerImpl>(),
            di::bind<authorship::Proposer>.template to<authorship::ProposerImpl>(),
            di::bind<authorship::BlockBuilder>.template to<authorship::BlockBuilderImpl>(),
            di::bind<authorship::BlockBuilderFactory>.template to<authorship::BlockBuilderFactoryImpl>(),
            bind_by_lambda<storage::SpacedStorage>([](const auto &injector) {
              auto& config =
                  injector
                      .template create<application::AppConfiguration const &>();
              auto chain_spec =
                  injector.template create<sptr<application::ChainSpec>>();
              BOOST_ASSERT(  // since rocksdb is the only possible option now
                  config.storageBackend()
                  == application::AppConfiguration::StorageBackend::RocksDB);
              return get_rocks_db(config, chain_spec);
            }),
            bind_by_lambda<blockchain::BlockStorage>([](const auto &injector) {
              auto root_res =
                  injector::calculate_genesis_state(
                      injector
                          .template create<const application::ChainSpec &>(),
                      injector
                          .template create<const crypto::Hasher &>(),
                      injector.template create<runtime::RuntimeInstancesPool&>(),
                      injector
                          .template create<storage::trie::TrieSerializer &>(),
                      injector.template create<
                          sptr<runtime::RuntimePropertiesCache>>());
              if (root_res.has_error()) {
                throw std::runtime_error{fmt::format("Failed to calculate genesis state: {}", root_res.error())};
              }
              const auto &hasher =
                  injector.template create<sptr<crypto::Hasher>>();
              const auto &storage =
                  injector.template create<sptr<storage::SpacedStorage>>();
              return blockchain::BlockStorageImpl::create(root_res.value(), storage, hasher)
                  .value();
            }),
            di::bind<blockchain::JustificationStoragePolicy>.template to<blockchain::JustificationStoragePolicyImpl>(),
            bind_by_lambda<blockchain::BlockTree>(
                [](const auto &injector) { return get_block_tree(injector); }),
            di::bind<blockchain::BlockHeaderRepository>.template to<blockchain::BlockHeaderRepositoryImpl>(),
            di::bind<clock::SystemClock>.template to<clock::SystemClockImpl>(),
            di::bind<clock::SteadyClock>.template to<clock::SteadyClockImpl>(),
            di::bind<clock::Timer>.template to<clock::BasicWaitableTimer>(),
            di::bind<network::Synchronizer>.template to<network::SynchronizerImpl>(),
            di::bind<consensus::grandpa::IVerifiedJustificationQueue>.template to<consensus::grandpa::VerifiedJustificationQueue>(),
            di::bind<consensus::grandpa::Environment>.template to<consensus::grandpa::EnvironmentImpl>(),
            di::bind<parachain::IApprovedAncestor>.template to<parachain::ApprovalDistribution>(),
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
            bind_by_lambda<crypto::KeyFileStorage>([](const auto &injector) {
              const auto &config =
                  injector
                      .template create<application::AppConfiguration const &>();
              auto chain_spec =
                  injector.template create<sptr<application::ChainSpec>>();

              return get_key_file_storage(config, chain_spec);
            }),
            di::bind<crypto::KeySuiteStore<crypto::Sr25519Provider>>.template to<crypto::KeySuiteStoreImpl<crypto::Sr25519Provider>>(),
            di::bind<crypto::KeySuiteStore<crypto::Ed25519Provider>>.template to<crypto::KeySuiteStoreImpl<crypto::Ed25519Provider>>(),
            di::bind<crypto::KeySuiteStore<crypto::EcdsaProvider>>.template to<crypto::KeySuiteStoreImpl<crypto::EcdsaProvider>>(),
            di::bind<crypto::KeySuiteStore<crypto::BandersnatchProvider>>.template to<crypto::KeySuiteStoreImpl<crypto::BandersnatchProvider>>(),
            di::bind<host_api::HostApiFactory>.template to<host_api::HostApiFactoryImpl>(),
            makeRuntimeInjector(config->runtimeExecMethod(), config->runtimeInterpreter()),
            di::bind<transaction_pool::TransactionPool>.template to<transaction_pool::TransactionPoolImpl>(),
            di::bind<transaction_pool::PoolModerator>.template to<transaction_pool::PoolModeratorImpl>(),
            di::bind<storage::changes_trie::ChangesTracker>.template to<storage::changes_trie::StorageChangesTrackerImpl>(),
            di::bind<network::StateProtocolObserver>.template to<network::StateProtocolObserverImpl>(),
            di::bind<network::SyncProtocolObserver>.template to<network::SyncProtocolObserverImpl>(),
            di::bind<network::DisputeRequestObserver>.template to<dispute::DisputeCoordinatorImpl>(),
            di::bind<parachain::AvailabilityStore>.template to<parachain::AvailabilityStoreImpl>(),
            di::bind<parachain::Fetch>.template to<parachain::FetchImpl>(),
            di::bind<parachain::Recovery>.template to<parachain::RecoveryImpl>(),
            di::bind<parachain::BitfieldStore>.template to<parachain::BitfieldStoreImpl>(),
            di::bind<parachain::BackingStore>.template to<parachain::BackingStoreImpl>(),
            di::bind<parachain::BackedCandidatesSource>.template to<parachain::ParachainProcessorImpl>(),
            di::bind<network::CanDisconnect>.template to<parachain::ParachainProcessorImpl>(),
            di::bind<parachain::Pvf>.template to<parachain::PvfImpl>(),
            di::bind<network::CollationObserver>.template to<parachain::ParachainObserverImpl>(),
            di::bind<network::ValidationObserver>.template to<parachain::ParachainObserverImpl>(),
            di::bind<network::ReqCollationObserver>.template to<parachain::ParachainObserverImpl>(),
            di::bind<network::ReqPovObserver>.template to<parachain::ParachainObserverImpl>(),
            di::bind<parachain::ParachainObserver>.template to<parachain::ParachainObserverImpl>(),
            bind_by_lambda<storage::trie::TrieStorageBackend>(
                [](const auto &injector) {
                  auto storage =
                      injector.template create<sptr<storage::SpacedStorage>>();
                  return get_trie_storage_backend(
                    storage
                  );
                }),
            bind_by_lambda<storage::trie::TrieStorage>([](const auto
                                                              &injector) {
              return storage::trie::TrieStorageImpl::createEmpty(
                         injector.template create<
                             sptr<storage::trie::PolkadotTrieFactory>>(),
                         injector.template create<sptr<storage::trie::Codec>>(),
                         injector.template create<
                             sptr<storage::trie::TrieSerializer>>(),
                         injector.template create<
                             sptr<storage::trie_pruner::TriePruner>>())
                  .value();
            }),
            di::bind<storage::trie::PolkadotTrieFactory>.template to<storage::trie::PolkadotTrieFactoryImpl>(),
            bind_by_lambda<storage::trie::Codec>([](const auto&) {
              return std::make_shared<storage::trie::PolkadotCodec>(crypto::blake2b<32>);
            }),
            di::bind<storage::trie::TrieSerializer>.template to<storage::trie::TrieSerializerImpl>(),
            bind_by_lambda<storage::trie_pruner::TriePruner>(
                [](const auto &injector)
                    -> sptr<storage::trie_pruner::TriePruner> {
                  auto &config =
                      injector.template create<
                          application::AppConfiguration const &>();
                  if (config.statePruningDepth() == std::nullopt
                      && !config.shouldPruneDiscardedStates()) {
                    return std::make_shared<
                        storage::trie_pruner::IdleTriePruner>();
                  }
                  return injector.template create<
                      sptr<storage::trie_pruner::TriePrunerImpl>>();
                }),
            di::bind<runtime::RuntimeContextFactory>.template to<runtime::RuntimeContextFactoryImpl>(),
            di::bind<runtime::RuntimeCodeProvider>.template to<runtime::StorageCodeProvider>(),
            bind_by_lambda<application::ChainSpec>([](const auto &injector) {
              auto &config =
                  injector
                      .template create<application::AppConfiguration const &>();
              return get_chain_spec(config);
            }),
            di::bind<network::ExtrinsicObserver>.template to<network::ExtrinsicObserverImpl>(),
            di::bind<consensus::grandpa::AuthorityManager>.template to<consensus::grandpa::AuthorityManagerImpl>(),
            di::bind<network::PeerManager>.template to<network::PeerManagerImpl>(),
            di::bind<network::Router>.template to<network::RouterLibp2p>(),
            di::bind<consensus::BlockHeaderAppender>.template to<consensus::BlockHeaderAppenderImpl>(),
            di::bind<consensus::BlockExecutor>.template to<consensus::BlockExecutorImpl>(),
            di::bind<consensus::grandpa::Grandpa>.template to<consensus::grandpa::GrandpaImpl>(),
            di::bind<consensus::grandpa::JustificationObserver>.template to<consensus::grandpa::GrandpaImpl>(),
            di::bind<consensus::grandpa::RoundObserver>.template to<consensus::grandpa::GrandpaImpl>(),
            di::bind<consensus::grandpa::CatchUpObserver>.template to<consensus::grandpa::GrandpaImpl>(),
            di::bind<consensus::grandpa::NeighborObserver>.template to<consensus::grandpa::GrandpaImpl>(),
            di::bind<consensus::grandpa::GrandpaObserver>.template to<consensus::grandpa::GrandpaImpl>(),
            di::bind<network::BlockAnnounceTransmitter>.template to<network::BlockAnnounceTransmitterImpl>(),
            di::bind<network::GrandpaTransmitter>.template to<network::GrandpaTransmitterImpl>(),
            di::bind<network::TransactionsTransmitter>.template to<network::TransactionsTransmitterImpl>(),
            bind_by_lambda<primitives::GenesisBlockHeader>(
                [](const auto &injector) {
                  return get_genesis_block_header(injector);
                }),
            di::bind<telemetry::TelemetryService>.template to<telemetry::TelemetryServiceImpl>(),
            di::bind<api::InternalApi>.template to<api::InternalApiImpl>(),
            di::bind<consensus::babe::BabeConfigRepository>.template to<consensus::babe::BabeConfigRepositoryImpl>(),
            di::bind<authority_discovery::Query>.template to<authority_discovery::QueryImpl>(),
            di::bind<libp2p::protocol::kademlia::Validator>.template to<authority_discovery::QueryImpl>()[boost::di::override],
            di::bind<crypto::SessionKeys>.template to<crypto::SessionKeysImpl>(),
            di::bind<network::SyncProtocol>.template to<network::SyncProtocolImpl>(),
            di::bind<network::StateProtocol>.template to<network::StateProtocolImpl>(),
            di::bind<network::BeefyProtocol>.template to<network::BeefyProtocolImpl>(),
            di::bind<consensus::beefy::FetchJustification>.template to<network::BeefyJustificationProtocol>(),
            di::bind<network::Beefy>.template to<network::BeefyImpl>(),
            di::bind<consensus::babe::BabeLottery>.template to<consensus::babe::BabeLotteryImpl>(),
            di::bind<network::BlockAnnounceObserver>.template to<consensus::TimelineImpl>(),
            di::bind<dispute::DisputeCoordinator>.template to<dispute::DisputeCoordinatorImpl>(),
            di::bind<dispute::Storage>.template to<dispute::StorageImpl>(),

            di::bind<consensus::ProductionConsensus *[]>()  // NOLINT
                .template to<consensus::babe::Babe
                             //,consensus::aura::Aura
                             //,consensus::sassafras::Sassafras
                            >(),
            di::bind<consensus::FinalityConsensus *[]>()  // NOLINT
                .template to<
                             consensus::grandpa::Grandpa
                            >(),
            di::bind<consensus::ConsensusSelector>.template to<consensus::ConsensusSelectorImpl>(),
            di::bind<consensus::SlotsUtil>.template to<consensus::SlotsUtilImpl>(),
            di::bind<consensus::Timeline>.template to<consensus::TimelineImpl>(),
            di::bind<consensus::babe::BabeBlockValidator>.template to<consensus::babe::BabeBlockValidatorImpl>(),
            di::bind<crypto::BandersnatchProvider>.template to<crypto::BandersnatchProviderImpl>(),
            di::bind<crypto::EllipticCurves>.template to<crypto::EllipticCurvesImpl>(),
            di::bind<network::FetchChunkProtocol>.template to<network::FetchChunkProtocolImpl>(),
            di::bind<network::FetchChunkProtocolObsolete>.template to<network::FetchChunkProtocolObsoleteImpl>(),
            di::bind<network::FetchAvailableDataProtocol>.template to<network::FetchAvailableDataProtocolImpl>(),
            di::bind<network::WarpProtocol>.template to<network::WarpProtocolImpl>(),
            di::bind<network::SendDisputeProtocol>.template to<network::SendDisputeProtocolImpl>(),
            bind_by_lambda<libp2p::protocol::IdentifyConfig>(
                [](const auto &injector) {
                  return get_identify_config();
                }),

            // user-defined overrides...
            std::forward<decltype(args)>(args)...);
    // clang-format on
  }

  template <typename... Ts>
  auto makeKagomeNodeInjector(sptr<application::AppConfiguration> app_config,
                              Ts &&...args) {
    return di::make_injector<boost::di::extension::shared_config>(
        makeApplicationInjector(std::move(app_config)),

        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }

}  // namespace

namespace kagome::injector {

  class KagomeNodeInjectorImpl {
   public:
    using Injector =
        decltype(makeKagomeNodeInjector(sptr<application::AppConfiguration>()));

    explicit KagomeNodeInjectorImpl(Injector injector)
        : injector_{std::move(injector)} {}
    Injector injector_;
  };

  KagomeNodeInjector::KagomeNodeInjector(
      sptr<application::AppConfiguration> app_config)
      : pimpl_{std::make_unique<KagomeNodeInjectorImpl>(
            makeKagomeNodeInjector(std::move(app_config)))} {}

  sptr<application::AppConfiguration> KagomeNodeInjector::injectAppConfig() {
    return pimpl_->injector_
        .template create<sptr<application::AppConfiguration>>();
  }

  sptr<application::ChainSpec> KagomeNodeInjector::injectChainSpec() {
    return pimpl_->injector_.template create<sptr<application::ChainSpec>>();
  }

  std::shared_ptr<blockchain::BlockStorage>
  KagomeNodeInjector::injectBlockStorage() {
    return pimpl_->injector_.template create<sptr<blockchain::BlockStorage>>();
  }

  sptr<application::AppStateManager>
  KagomeNodeInjector::injectAppStateManager() {
    return pimpl_->injector_
        .template create<sptr<application::AppStateManager>>();
  }

  sptr<boost::asio::io_context> KagomeNodeInjector::injectIoContext() {
    return pimpl_->injector_.template create<sptr<boost::asio::io_context>>();
  }

  sptr<metrics::Exposer> KagomeNodeInjector::injectOpenMetricsService() {
    // registry here is temporary, it initiates static global registry
    // and registers handler in there
    auto registry = metrics::createRegistry();
    auto handler = pimpl_->injector_.template create<sptr<metrics::Handler>>();
    registry->setHandler(*handler.get());
    auto exposer = pimpl_->injector_.template create<sptr<metrics::Exposer>>();
    exposer->setHandler(handler);
    return exposer;
  }

  sptr<network::Router> KagomeNodeInjector::injectRouter() {
    return pimpl_->injector_.template create<sptr<network::Router>>();
  }

  sptr<network::PeerManager> KagomeNodeInjector::injectPeerManager() {
    return pimpl_->injector_.template create<sptr<network::PeerManager>>();
  }

  sptr<api::ApiService> KagomeNodeInjector::injectRpcApiService() {
    return pimpl_->injector_.template create<sptr<api::ApiService>>();
  }

  std::shared_ptr<clock::SystemClock> KagomeNodeInjector::injectSystemClock() {
    return pimpl_->injector_.template create<sptr<clock::SystemClock>>();
  }

  std::shared_ptr<network::SyncProtocolObserver>
  KagomeNodeInjector::injectSyncObserver() {
    return pimpl_->injector_
        .template create<sptr<network::SyncProtocolObserver>>();
  }

  std::shared_ptr<network::StateProtocolObserver>
  KagomeNodeInjector::injectStateObserver() {
    return pimpl_->injector_
        .template create<sptr<network::StateProtocolObserver>>();
  }

  std::shared_ptr<parachain::ParachainObserver>
  KagomeNodeInjector::injectParachainObserver() {
    return pimpl_->injector_
        .template create<sptr<parachain::ParachainObserver>>();
  }

  std::shared_ptr<parachain::ParachainProcessorImpl>
  KagomeNodeInjector::injectParachainProcessor() {
    return pimpl_->injector_
        .template create<sptr<parachain::ParachainProcessorImpl>>();
  }

  std::shared_ptr<parachain::ApprovalDistribution>
  KagomeNodeInjector::injectApprovalDistribution() {
    return pimpl_->injector_
        .template create<sptr<parachain::ApprovalDistribution>>();
  }

  std::shared_ptr<network::DisputeRequestObserver>
  KagomeNodeInjector::injectDisputeRequestObserver() {
    return pimpl_->injector_.create<sptr<network::DisputeRequestObserver>>();
  }

  std::shared_ptr<dispute::DisputeCoordinator>
  KagomeNodeInjector::injectDisputeCoordinator() {
    return pimpl_->injector_.create<sptr<dispute::DisputeCoordinator>>();
  }

  std::shared_ptr<consensus::Timeline> KagomeNodeInjector::injectTimeline() {
    pimpl_->injector_.template create<sptr<consensus::ConsensusSelector>>();
    pimpl_->injector_.template create<sptr<consensus::SlotsUtil>>();
    return pimpl_->injector_.template create<sptr<consensus::Timeline>>();
  }

  std::shared_ptr<consensus::grandpa::Grandpa>
  KagomeNodeInjector::injectGrandpa() {
    return pimpl_->injector_
        .template create<sptr<consensus::grandpa::Grandpa>>();
  }

  std::shared_ptr<soralog::LoggingSystem>
  KagomeNodeInjector::injectLoggingSystem() {
    return std::make_shared<soralog::LoggingSystem>(
        std::make_shared<kagome::log::Configurator>(
            pimpl_->injector_
                .template create<sptr<libp2p::log::Configurator>>()));
  }

  std::shared_ptr<storage::trie::TrieStorage>
  KagomeNodeInjector::injectTrieStorage() {
    return pimpl_->injector_
        .template create<sptr<storage::trie::TrieStorage>>();
  }

  std::shared_ptr<metrics::MetricsWatcher>
  KagomeNodeInjector::injectMetricsWatcher() {
    return pimpl_->injector_.template create<sptr<metrics::MetricsWatcher>>();
  }

  std::shared_ptr<telemetry::TelemetryService>
  KagomeNodeInjector::injectTelemetryService() {
    return pimpl_->injector_
        .template create<sptr<telemetry::TelemetryService>>();
  }

  std::shared_ptr<application::mode::PrintChainInfoMode>
  KagomeNodeInjector::injectPrintChainInfoMode() {
    return pimpl_->injector_
        .create<sptr<application::mode::PrintChainInfoMode>>();
  }

  sptr<application::mode::PrecompileWasmMode>
  KagomeNodeInjector::injectPrecompileWasmMode() {
    return pimpl_->injector_
        .create<sptr<application::mode::PrecompileWasmMode>>();
  }

  std::shared_ptr<application::mode::RecoveryMode>
  KagomeNodeInjector::injectRecoveryMode() {
    return pimpl_->injector_
        .template create<sptr<application::mode::RecoveryMode>>();
  }

  std::shared_ptr<blockchain::BlockTree> KagomeNodeInjector::injectBlockTree() {
    return pimpl_->injector_.template create<sptr<blockchain::BlockTree>>();
  }

  std::shared_ptr<runtime::Executor> KagomeNodeInjector::injectExecutor() {
    return pimpl_->injector_.template create<sptr<runtime::Executor>>();
  }

  std::shared_ptr<storage::SpacedStorage> KagomeNodeInjector::injectStorage() {
    return pimpl_->injector_.template create<sptr<storage::SpacedStorage>>();
  }

  std::shared_ptr<authority_discovery::AddressPublisher>
  KagomeNodeInjector::injectAddressPublisher() {
    return pimpl_->injector_
        .template create<sptr<authority_discovery::AddressPublisher>>();
  }

  std::shared_ptr<benchmark::BlockExecutionBenchmark>
  KagomeNodeInjector::injectBlockBenchmark() {
    return pimpl_->injector_
        .template create<sptr<benchmark::BlockExecutionBenchmark>>();
  }

  std::shared_ptr<Watchdog> KagomeNodeInjector::injectWatchdog() {
    return pimpl_->injector_.template create<sptr<Watchdog>>();
  }

  std::shared_ptr<common::MainThreadPool>
  KagomeNodeInjector::injectMainThreadPool() {
    return pimpl_->injector_.template create<sptr<common::MainThreadPool>>();
  }

  void KagomeNodeInjector::kademliaRandomWalk() {
    pimpl_->injector_.create<sptr<KademliaRandomWalk>>();
  }
}  // namespace kagome::injector
