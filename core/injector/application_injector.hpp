/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP
#define KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP

#include <memory>

#define BOOST_DI_CFG_DIAGNOSTICS_LEVEL 2

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

  sptr<api::HttpListenerImpl> get_jrpc_api_http_listener(
      application::AppConfiguration const &config,
      sptr<application::AppStateManager> app_state_manager,
      sptr<api::RpcContext> context,
      api::HttpSession::Configuration http_session_config);

  sptr<api::WsListenerImpl> get_jrpc_api_ws_listener(
      api::WsSession::Configuration ws_session_config,
      sptr<api::RpcContext> context,
      sptr<application::AppStateManager> app_state_manager,
      const boost::asio::ip::tcp::endpoint &endpoint);

  sptr<blockchain::BlockStorage> get_block_storage(
      sptr<crypto::Hasher> hasher,
      sptr<storage::BufferStorage> db,
      sptr<storage::trie::TrieStorage> trie_storage,
      sptr<runtime::GrandpaApi> grandpa_api);

  sptr<host_api::HostApiFactoryImpl> get_host_api_factory(
      sptr<storage::changes_trie::ChangesTracker> tracker,
      sptr<crypto::Sr25519Provider> sr25519_provider,
      sptr<crypto::Ed25519Provider> ed25519_provider,
      sptr<crypto::Secp256k1Provider> secp256k1_provider,
      sptr<crypto::Hasher> hasher,
      sptr<crypto::CryptoStore> crypto_store,
      sptr<crypto::Bip39Provider> bip39_provider);

  sptr<storage::trie::TrieStorageBackendImpl> get_trie_storage_backend(
      sptr<storage::BufferStorage> storage);

  sptr<storage::trie::TrieStorageImpl> get_trie_storage_impl(
      sptr<storage::trie::PolkadotTrieFactory> factory,
      sptr<storage::trie::Codec> codec,
      sptr<storage::trie::TrieSerializer> serializer,
      sptr<storage::changes_trie::ChangesTracker> tracker);

  sptr<storage::trie::TrieStorage> get_trie_storage(
      sptr<application::ChainSpec> configuration_storage,
      sptr<storage::trie::TrieStorageImpl> trie_storage);

  sptr<storage::BufferStorage> get_level_db(
      application::AppConfiguration const &app_config,
      sptr<application::ChainSpec> chain_spec);

  std::shared_ptr<application::ChainSpec> get_genesis_config(
      application::AppConfiguration const &config);

  sptr<primitives::BabeConfiguration> get_babe_configuration(
      sptr<runtime::BabeApi> babe_api);

  consensus::SlotsStrategy get_slots_strategy(
      const application::AppConfiguration &app_config);

  sptr<crypto::KeyFileStorage> get_key_file_storage(
      application::AppConfiguration const &config,
      sptr<application::ChainSpec> genesis_config);

  sptr<consensus::grandpa::FinalizationObserver> get_finalization_observer(
      sptr<authority::AuthorityManagerImpl> authority_manager);

  const sptr<libp2p::crypto::KeyPair> &get_peer_keypair(
      const application::AppConfiguration &app_config,
      const crypto::Ed25519Provider &crypto_provider,
      const crypto::CryptoStore &crypto_store);

  sptr<libp2p::protocol::kademlia::Config> get_kademlia_config(
      const application::ChainSpec &chain_spec);

  template <typename Injector>
  sptr<api::ApiServiceImpl> get_jrpc_api_service(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<api::ApiServiceImpl>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    const std::shared_ptr<application::AppStateManager> &asmgr =
        injector.template create<
            const std::shared_ptr<application::AppStateManager> &>();
    auto thread_pool = injector.template create<sptr<api::RpcThreadPool>>();
    auto server = injector.template create<sptr<api::JRpcServer>>();
    auto listeners =
        injector.template create<api::ApiServiceImpl::ListenerList>();
    const auto &processors =
        injector.template create<const api::ApiServiceImpl::ProcessorSpan &>();
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
    auto service =
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
    initialized =
        service;  // injector.template create<sptr<api::ApiServiceImpl>>();

    auto state_api = injector.template create<std::shared_ptr<api::StateApi>>();
    state_api->setApiService(initialized.value());

    auto chain_api = injector.template create<std::shared_ptr<api::ChainApi>>();
    chain_api->setApiService(initialized.value());

    auto author_api =
        injector.template create<std::shared_ptr<api::AuthorApi>>();
    author_api->setApiService(initialized.value());

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
        injector.template create<sptr<libp2p::protocol::Ping>>());
    //initialized = injector.template create<sptr<network::RouterLibp2p>>();
    return initialized.value();
  }

  template <class Injector>
  sptr<network::PeerManager> get_peer_manager(const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<network::PeerManager>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    /*
    const Config &config,
    std::shared_ptr<Host> host,
    std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentRoutingTable> content_routing_table,
    std::shared_ptr<PeerRoutingTable> peer_routing_table,
    std::shared_ptr<Validator> validator,
    std::shared_ptr<Scheduler> scheduler,
    std::shared_ptr<event::Bus> bus,
    std::shared_ptr<crypto::random::RandomGenerator> random_generator
    */
    auto config =
        injector.template create<const libp2p::protocol::kademlia::Config &>();
    /*
    auto kademlia =
        injector.template create<sptr<libp2p::protocol::kademlia::Kademlia>>();
    */
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
        injector.template create<sptr<network::SyncClientsSet>>());
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

  template <typename... Ts>
  auto makeApplicationInjector(const application::AppConfiguration &config,
                               Ts &&...args) {
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
          auto &chain_spec = inj.template create<application::ChainSpec &>();
          return get_kademlia_config(chain_spec);
        })[boost::di::override],

        di::bind<application::AppStateManager>.template to<application::AppStateManagerImpl>(),
        di::bind<application::AppConfiguration>.to(config),

        // compose peer keypair
        di::bind<libp2p::crypto::KeyPair>.to([](auto const &inj) {
          auto &app_config =
              inj.template create<const application::AppConfiguration &>();
          auto &crypto_provider =
              inj.template create<const crypto::Ed25519Provider &>();
          auto &crypto_store =
              inj.template create<const crypto::CryptoStore &>();
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
          auto &endpoint = app_config.rpcWsEndpoint();

          return get_jrpc_api_ws_listener(
              config, context, app_state_manager, endpoint);
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
        di::bind<storage::BufferStorage>.to([](const auto &injector) {
          const application::AppConfiguration &config =
              injector.template create<application::AppConfiguration const &>();
          auto genesis_config =
              injector.template create<sptr<application::ChainSpec>>();
          return get_level_db(config, genesis_config);
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
            [](auto const &inj) { return get_block_tree(inj); }),
        di::bind<blockchain::BlockHeaderRepository>.template to<blockchain::KeyValueBlockHeaderRepository>(),
        di::bind<clock::SystemClock>.template to<clock::SystemClockImpl>(),
        di::bind<clock::SteadyClock>.template to<clock::SteadyClockImpl>(),
        di::bind<clock::Timer>.template to<clock::BasicWaitableTimer>(),
        di::bind<primitives::BabeConfiguration>.to([](auto const &injector) {
          auto babe_api = injector.template create<sptr<runtime::BabeApi>>();
          return get_babe_configuration(babe_api);
        }),
        di::bind<consensus::BabeSynchronizer>.template to<consensus::BabeSynchronizerImpl>(),
        di::bind<consensus::SlotsStrategy>.template to(
            [](const auto &injector) {
              const application::AppConfiguration &config =
                  injector
                      .template create<const application::AppConfiguration &>();
              return get_slots_strategy(config);
            }),
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
          auto genesis_config =
              injector.template create<sptr<application::ChainSpec>>();

          return get_key_file_storage(config, genesis_config);
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
        di::bind<storage::trie::TrieStorageBackend>.to([](auto const &inj) {
          auto storage = inj.template create<sptr<storage::BufferStorage>>();
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
          return get_genesis_config(config);
        }),
        di::bind<network::ExtrinsicObserver>.template to<network::ExtrinsicObserverImpl>(),
        di::bind<network::ExtrinsicGossiper>.template to<network::GossiperBroadcast>(),
        di::bind<authority::AuthorityUpdateObserver>.template to<authority::AuthorityManagerImpl>(),
        di::bind<authority::AuthorityManager>.template to<authority::AuthorityManagerImpl>(),
        di::bind<consensus::grandpa::FinalizationObserver>.to(
            [](auto const &injector) {
              auto authority_manager = injector.template create<
                  std::shared_ptr<authority::AuthorityManagerImpl>>();
              return get_finalization_observer(authority_manager);
            }),
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
