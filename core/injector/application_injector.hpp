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

#include "api/extrinsic/extrinsic_jrpc_processor.hpp"
#include "api/extrinsic/impl/extrinsic_api_impl.hpp"
#include "api/service/api_service.hpp"
#include "api/state/impl/state_api_impl.hpp"
#include "api/state/state_jrpc_processor.hpp"
#include "api/transport/impl/http/http_listener_impl.hpp"
#include "api/transport/impl/http/http_session.hpp"
#include "api/transport/impl/ws/ws_listener_impl.hpp"
#include "api/transport/impl/ws/ws_session.hpp"
#include "application/impl/configuration_storage_impl.hpp"
#include "application/impl/local_key_storage.hpp"
#include "authorship/impl/block_builder_factory_impl.hpp"
#include "authorship/impl/block_builder_impl.hpp"
#include "authorship/impl/proposer_impl.hpp"
#include "blockchain/impl/block_tree_impl.hpp"
#include "blockchain/impl/changes_trie_builder_impl.hpp"
#include "blockchain/impl/key_value_block_header_repository.hpp"
#include "blockchain/impl/key_value_block_storage.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "boost/di/extension/injections/extensible_injector.hpp"
#include "clock/impl/basic_waitable_timer.hpp"
#include "clock/impl/clock_impl.hpp"
#include "common/outcome_throw.hpp"
#include "consensus/babe/babe_lottery.hpp"
#include "consensus/babe/common.hpp"
#include "consensus/babe/impl/babe_impl.hpp"
#include "consensus/babe/impl/babe_lottery_impl.hpp"
#include "consensus/babe/impl/babe_synchronizer_impl.hpp"
#include "consensus/babe/impl/epoch_storage_impl.hpp"
#include "consensus/grandpa/chain.hpp"
#include "consensus/grandpa/gossiper.hpp"
#include "consensus/grandpa/impl/environment_impl.hpp"
#include "consensus/grandpa/impl/launcher_impl.hpp"
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
#include "extensions/impl/extension_factory_impl.hpp"
#include "network/impl/gossiper_broadcast.hpp"
#include "network/impl/router_libp2p.hpp"
#include "network/sync_protocol_client.hpp"
#include "network/sync_protocol_observer.hpp"
#include "network/types/sync_clients_set.hpp"
#include "runtime/binaryen/runtime_api/babe_api_impl.hpp"
#include "runtime/binaryen/runtime_api/block_builder_impl.hpp"
#include "runtime/binaryen/runtime_api/core_impl.hpp"
#include "runtime/binaryen/runtime_api/grandpa_impl.hpp"
#include "runtime/binaryen/runtime_api/metadata_impl.hpp"
#include "runtime/binaryen/runtime_api/offchain_worker_impl.hpp"
#include "runtime/binaryen/runtime_api/parachain_host_impl.hpp"
#include "runtime/binaryen/runtime_api/tagged_transaction_queue_impl.hpp"
#include "runtime/common/storage_wasm_provider.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/impl/in_memory_trie_db_factory.hpp"
#include "storage/trie/impl/polkadot_codec.hpp"
#include "storage/trie/impl/polkadot_node.hpp"
#include "storage/trie/impl/polkadot_trie_db.hpp"
#include "storage/trie/impl/readonly_trie_factory_impl.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/trie_db_reader.hpp"
#include "storage/trie_db_overlay/impl/trie_db_overlay_impl.hpp"
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
  auto useConfig(C c) {
    return boost::di::bind<std::decay_t<C>>().template to(
        std::move(c))[boost::di::override];
  }

  template <class T>
  using sptr = std::shared_ptr<T>;

  template <class T>
  using uptr = std::unique_ptr<T>;

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

  // peer info getter
  auto get_peer_info = [](const auto &injector,
                          uint16_t p2p_port) -> sptr<libp2p::peer::PeerInfo> {
    static auto initialized =
        boost::optional<sptr<libp2p::peer::PeerInfo>>(boost::none);
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
        "/ip4/127.0.0.1/tcp/" + std::to_string(p2p_port);
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

  auto get_boot_nodes = [](const auto &injector) -> sptr<network::PeerList> {
    static auto initialized =
        boost::optional<sptr<network::PeerList>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    auto &cfg = injector.template create<application::ConfigurationStorage &>();

    initialized = std::make_shared<network::PeerList>(cfg.getBootNodes());
    return initialized.value();
  };

  auto get_jrpc_api_service =
      [](const auto &injector) -> sptr<api::ApiService> {
    static auto initialized =
        boost::optional<sptr<api::ApiService>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    std::vector<std::shared_ptr<api::Listener>> listeners{
        injector.template create<std::shared_ptr<api::HttpListenerImpl>>(),
        injector.template create<std::shared_ptr<api::WsListenerImpl>>(),
    };
    auto server = injector.template create<std::shared_ptr<api::JRpcServer>>();
    std::vector<std::shared_ptr<api::JRpcProcessor>> processors{
        injector.template create<std::shared_ptr<api::StateJrpcProcessor>>(),
        injector
            .template create<std::shared_ptr<api::ExtrinsicJRpcProcessor>>()};
    initialized =
        std::make_shared<api::ApiService>(listeners, server, processors);
    return initialized.value();
  };

  // jrpc api listener (over HTTP) getter
  auto get_jrpc_api_http_listener =
      [](const auto &injector,
         uint16_t rpc_port) -> sptr<api::HttpListenerImpl> {
    static auto initialized =
        boost::optional<sptr<api::HttpListenerImpl>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    auto &context = injector.template create<boost::asio::io_context &>();
    auto extrinsic_tcp_version = boost::asio::ip::tcp::v4();
    api::HttpListenerImpl::Configuration listener_config{
        boost::asio::ip::tcp::endpoint{extrinsic_tcp_version, rpc_port}};
    auto &&http_session_config =
        injector.template create<api::HttpSession::Configuration>();

    initialized = std::make_shared<api::HttpListenerImpl>(
        context, listener_config, http_session_config);
    return initialized.value();
  };

  // jrpc api listener (over Websockets) getter
  auto get_jrpc_api_ws_listener =
      [](const auto &injector, uint16_t rpc_port) -> sptr<api::WsListenerImpl> {
    static auto initialized =
        boost::optional<sptr<api::WsListenerImpl>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    auto &context = injector.template create<boost::asio::io_context &>();
    auto extrinsic_tcp_version = boost::asio::ip::tcp::v4();
    api::WsListenerImpl::Configuration listener_config{
        boost::asio::ip::tcp::endpoint{extrinsic_tcp_version, rpc_port}};
    auto &&ws_session_config =
        injector.template create<api::WsSession::Configuration>();

    initialized = std::make_shared<api::WsListenerImpl>(
        context, listener_config, ws_session_config);
    return initialized.value();
  };

  // block storage getter
  auto get_block_storage =
      [](const auto &injector) -> sptr<blockchain::BlockStorage> {
    static auto initialized =
        boost::optional<sptr<blockchain::BlockStorage>>(boost::none);

    if (initialized) {
      return initialized.value();
    }
    auto &&hasher = injector.template create<sptr<crypto::Hasher>>();

    const auto &db = injector.template create<sptr<storage::BufferStorage>>();

    const auto &trie_db =
        injector.template create<sptr<storage::trie::TrieDb>>();

    auto storage = blockchain::KeyValueBlockStorage::createWithGenesis(
        trie_db->getRootHash(),
        db,
        hasher,
        [&db, &injector](const primitives::Block &genesis_block) {
          // handle genesis initialization, which happens when there is not
          // authorities and last completed round in the storage
          if (not db->get(storage::kAuthoritySetKey)) {
            // insert authorities
            auto grandpa_api =
                injector.template create<sptr<runtime::Grandpa>>();
            const auto &weighted_authorities_res = grandpa_api->authorities(
                primitives::BlockId(primitives::BlockNumber{0}));
            BOOST_ASSERT_MSG(weighted_authorities_res,
                             "grandpa_api_->authorities failed");
            const auto &weighted_authorities = weighted_authorities_res.value();
            consensus::grandpa::VoterSet voters{0};
            for (const auto &weighted_authority : weighted_authorities) {
              voters.insert(weighted_authority.id.id, 1);
              spdlog::debug("Added to grandpa authorities: {}",
                            weighted_authority.id.id.toHex());
            }
            BOOST_ASSERT_MSG(voters.size() != 0, "Grandpa voters are empty");
            auto authorities_put_res =
                db->put(storage::kAuthoritySetKey,
                        common::Buffer(scale::encode(voters).value()));
            if (not authorities_put_res) {
              BOOST_ASSERT_MSG(false, "Could not insert authorities");
              std::exit(1);
            }

            // insert last completed round
            consensus::grandpa::CompletedRound zero_round;
            zero_round.round_number = 0;
            const auto &hasher = injector.template create<crypto::Hasher &>();
            auto genesis_hash =
                hasher.blake2b_256(scale::encode(genesis_block.header).value());
            spdlog::debug("Genesis hash in injector: {}", genesis_hash.toHex());
            zero_round.state.prevote_ghost =
                consensus::grandpa::Prevote(0, genesis_hash);
            zero_round.state.estimate = primitives::BlockInfo(0, genesis_hash);
            zero_round.state.finalized = primitives::BlockInfo(0, genesis_hash);
            auto completed_round_put_res =
                db->put(storage::kSetStateKey,
                        common::Buffer(scale::encode(zero_round).value()));
            if (not completed_round_put_res) {
              BOOST_ASSERT_MSG(false, "Could not insert completed round");
              std::exit(1);
            }
          }
        });
    if (storage.has_error()) {
      common::raise(storage.error());
    }
    initialized = storage.value();
    return initialized.value();
  };

  // block tree getter
  auto get_block_tree =
      [](const auto &injector) -> sptr<blockchain::BlockTree> {
    static auto initialized =
        boost::optional<sptr<blockchain::BlockTree>>(boost::none);

    if (initialized) {
      return initialized.value();
    }
    auto header_repo =
        injector.template create<sptr<blockchain::BlockHeaderRepository>>();

    auto &&storage = injector.template create<sptr<blockchain::BlockStorage>>();

    // block id is zero for genesis launch
    const primitives::BlockId block_id = 0;

    auto &&hasher = injector.template create<sptr<crypto::Hasher>>();

    auto &&tree = blockchain::BlockTreeImpl::create(
        std::move(header_repo), storage, block_id, std::move(hasher));
    if (!tree) {
      common::raise(tree.error());
    }
    initialized = tree.value();
    return initialized.value();
  };

  // polkadot trie db getter
  auto get_polkadot_trie_db_backend =
      [](const auto &injector) -> sptr<storage::trie::TrieStorageBackendImpl> {
    static auto initialized =
        boost::optional<sptr<storage::trie::TrieStorageBackendImpl>>(boost::none);

    if (initialized) {
      return initialized.value();
    }
    auto storage = injector.template create<sptr<storage::BufferStorage>>();
    using blockchain::prefix::TRIE_NODE;
    auto backend = std::make_shared<storage::trie::TrieStorageBackendImpl>(
        storage, common::Buffer{TRIE_NODE});
    initialized = backend;
    return backend;
  };

  auto get_polkadot_trie_db =
      [](const auto &injector) -> sptr<storage::trie::PolkadotTrieDb> {
    static auto initialized =
        boost::optional<sptr<storage::trie::PolkadotTrieDb>>(boost::none);

    if (initialized) {
      return initialized.value();
    }
    auto backend =
        injector.template create<sptr<storage::trie::TrieStorageBackend>>();
    sptr<storage::trie::PolkadotTrieDb> polkadot_trie_db =
        storage::trie::PolkadotTrieDb::createEmpty(backend);
    initialized = polkadot_trie_db;
    return polkadot_trie_db;
  };

  // trie db getter
  auto get_trie_db = [](const auto &injector) -> sptr<storage::trie::TrieDb> {
    static auto initialized =
        boost::optional<sptr<storage::trie::TrieDb>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    auto configuration_storage =
        injector.template create<sptr<application::ConfigurationStorage>>();
    const auto &genesis_raw_configs = configuration_storage->getGenesis();

    auto trie_db =
        injector.template create<sptr<storage::trie::PolkadotTrieDb>>();
    for (const auto &[key, val] : genesis_raw_configs) {
      spdlog::debug("Key: {} ({}), Val: {}",
                    key.toHex(),
                    key.data(),
                    val.toHex().substr(0, 200));
      if (auto res = trie_db->put(key, val); not res) {
        common::raise(res.error());
      }
    }
    initialized = trie_db;
    return trie_db;
  };

  // trie db overlay getter
  auto get_trie_db_overlay =
      [](const auto &injector) -> sptr<storage::trie_db_overlay::TrieDbOverlayImpl> {
    static auto initialized =
        boost::optional<sptr<storage::trie_db_overlay::TrieDbOverlayImpl>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    auto trie_db_overlay =
        std::make_shared<storage::trie_db_overlay::TrieDbOverlayImpl>(
            get_trie_db(injector));
    return trie_db_overlay;
  };

  // level db getter
  template <typename Injector>
  sptr<storage::BufferStorage> get_level_db(std::string_view leveldb_path,
                                            const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<storage::BufferStorage>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    auto options = leveldb::Options{};
    options.create_if_missing = true;
    auto db = storage::LevelDB::create(leveldb_path, options);
    if (!db) {
      common::raise(db.error());
    }
    initialized = db.value();
    return initialized.value();
  };

  // configuration storage getter
  template <typename Injector>
  std::shared_ptr<application::ConfigurationStorage> get_configuration_storage(
      std::string_view genesis_path, const Injector &injector) {
    static auto initialized =
        boost::optional<sptr<application::ConfigurationStorage>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    auto config_storage_res = application::ConfigurationStorageImpl::create(
        std::string(genesis_path));
    if (config_storage_res.has_error()) {
      common::raise(config_storage_res.error());
    }
    initialized = config_storage_res.value();
    return config_storage_res.value();
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

  auto get_sync_clients_set =
      [](const auto &injector) -> sptr<network::SyncClientsSet> {
    static auto initialized =
        boost::optional<sptr<network::SyncClientsSet>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    auto configuration_storage =
        injector.template create<sptr<application::ConfigurationStorage>>();
    auto peer_infos = configuration_storage->getBootNodes().peers;

    auto host = injector.template create<sptr<libp2p::Host>>();
    auto block_tree = injector.template create<sptr<blockchain::BlockTree>>();
    auto block_header_repository =
        injector.template create<sptr<blockchain::BlockHeaderRepository>>();

    auto res = std::make_shared<network::SyncClientsSet>();
    auto current_peer_synchronizer =
        injector.template create<sptr<consensus::Synchronizer>>();
    res->clients.insert(current_peer_synchronizer);

    auto current_peer_info = injector.template create<libp2p::peer::PeerInfo>();
    for (const auto &peer_info : peer_infos) {
      if (peer_info.id != current_peer_info.id) {
        res->clients.insert(std::make_shared<consensus::SynchronizerImpl>(
            *host,
            peer_info,
            block_tree,
            block_header_repository,
            injector.template create<consensus::SynchronizerConfig>()));
      }
    }
    initialized = res;
    return res;
  };

  auto get_babe = [](const auto &injector) -> sptr<consensus::Babe> {
    static auto initialized =
        boost::optional<sptr<consensus::BabeImpl>>(boost::none);
    if (initialized) {
      return *initialized;
    }

    initialized = std::make_shared<consensus::BabeImpl>(
        injector.template create<sptr<consensus::BabeLottery>>(),
        injector.template create<sptr<consensus::BabeSynchronizer>>(),
        injector.template create<sptr<consensus::BabeBlockValidator>>(),
        injector.template create<sptr<consensus::EpochStorage>>(),
        injector.template create<sptr<runtime::BabeApi>>(),
        injector.template create<sptr<runtime::Core>>(),
        injector.template create<sptr<authorship::Proposer>>(),
        injector.template create<sptr<blockchain::BlockTree>>(),
        injector.template create<sptr<network::BabeGossiper>>(),
        injector.template create<crypto::SR25519Keypair>(),
        injector.template create<sptr<clock::SystemClock>>(),
        injector.template create<sptr<crypto::Hasher>>(),
        injector.template create<uptr<clock::Timer>>());
    return *initialized;
  };

  auto get_babe_observer = get_babe;

  template <typename... Ts>
  auto makeApplicationInjector(const std::string &genesis_path,
                               const std::string &keystore_path,
                               const std::string &leveldb_path,
                               uint16_t p2p_port,
                               uint16_t rpc_http_port,
                               uint16_t rpc_ws_port,
                               Ts &&... args) {
    using namespace boost;  // NOLINT;

    // default values for configurations
    api::HttpSession::Configuration http_config{};
    api::WsSession::Configuration ws_config{};
    transaction_pool::PoolModeratorImpl::Params pool_moderator_config{};
    consensus::SynchronizerConfig synchronizer_config{};
    transaction_pool::TransactionPool::Limits tp_pool_limits{};
    return di::make_injector(
        // bind configs
        injector::useConfig(http_config),
        injector::useConfig(ws_config),
        injector::useConfig(pool_moderator_config),
        injector::useConfig(synchronizer_config),
        injector::useConfig(tp_pool_limits),

        // inherit host injector
        libp2p::injector::makeHostInjector(),
        // bind sr25519 keypair
        di::bind<crypto::SR25519Keypair>.to(std::move(get_sr25519_keypair)),
        // bind ed25519 keypair
        di::bind<crypto::ED25519Keypair>.to(std::move(get_ed25519_keypair)),
        // compose peer info
        di::bind<libp2p::peer::PeerInfo>.to([p2p_port](const auto &injector) {
          return get_peer_info(injector, p2p_port);
        }),
        // compose peer keypair
        di::bind<libp2p::crypto::KeyPair>.to(
            std::move(get_peer_keypair))[boost::di::override],
        // bind boot nodes
        di::bind<network::PeerList>.to(std::move(get_boot_nodes)),

        // bind io_context: 1 per injector
        di::bind<::boost::asio::io_context>.in(
            di::extension::shared)[boost::di::override],

        // bind interfaces
        di::bind<api::HttpListenerImpl>.to(
            [rpc_http_port](const auto &injector) {
              return get_jrpc_api_http_listener(injector, rpc_http_port);
            }),
        di::bind<api::WsListenerImpl>.to([rpc_ws_port](const auto &injector) {
          return get_jrpc_api_ws_listener(injector, rpc_ws_port);
        }),
        di::bind<storage::trie::ReadonlyTrieFactory>.template to<storage::trie::ReadonlyTrieFactoryImpl>(),
        di::bind<api::ExtrinsicApi>.template to<api::ExtrinsicApiImpl>(),
        di::bind<api::StateApi>.template to<api::StateApiImpl>(),
        di::bind<api::ApiService>.to(std::move(get_jrpc_api_service)),
        di::bind<api::JRpcServer>.template to<api::JRpcServerImpl>(),
        di::bind<authorship::Proposer>.template to<authorship::ProposerImpl>(),
        di::bind<authorship::BlockBuilder>.template to<authorship::BlockBuilderImpl>(),
        di::bind<authorship::BlockBuilderFactory>.template to<authorship::BlockBuilderFactoryImpl>(),
        di::bind<storage::BufferStorage>.to(
            [leveldb_path](const auto &injector) {
              return get_level_db(leveldb_path, injector);
            }),
        di::bind<blockchain::BlockStorage>.to(std::move(get_block_storage)),
        di::bind<blockchain::BlockTree>.to(std::move(get_block_tree)),
        di::bind<blockchain::BlockHeaderRepository>.template to<blockchain::KeyValueBlockHeaderRepository>(),
        di::bind<blockchain::ChangesTrieBuilder>.template to<blockchain::ChangesTrieBuilderImpl>(),
        di::bind<clock::SystemClock>.template to<clock::SystemClockImpl>(),
        di::bind<clock::SteadyClock>.template to<clock::SteadyClockImpl>(),
        di::bind<clock::Timer>.template to<clock::BasicWaitableTimer>(),
        di::bind<consensus::Babe>.to(std::move(get_babe)),
        di::bind<consensus::BabeSynchronizer>.template to<consensus::BabeSynchronizerImpl>(),
        di::bind<consensus::BabeLottery>.template to<consensus::BabeLotteryImpl>(),
        di::bind<network::BabeObserver>.to(std::move(get_babe_observer)),
        di::bind<consensus::grandpa::RoundObserver>.template to<consensus::grandpa::LauncherImpl>(),
        di::bind<consensus::grandpa::Launcher>.template to<consensus::grandpa::LauncherImpl>(),
        di::bind<consensus::grandpa::Environment>.template to<consensus::grandpa::EnvironmentImpl>(),
        di::bind<consensus::EpochStorage>.template to<consensus::EpochStorageImpl>(),
        di::bind<consensus::Synchronizer>.template to<consensus::SynchronizerImpl>(),
        di::bind<consensus::BlockValidator>.template to<consensus::BabeBlockValidator>(),
        di::bind<crypto::ED25519Provider>.template to<crypto::ED25519ProviderImpl>(),
        di::bind<crypto::Hasher>.template to<crypto::HasherImpl>(),
        di::bind<crypto::SR25519Provider>.template to<crypto::SR25519ProviderImpl>(),
        di::bind<crypto::VRFProvider>.template to<crypto::VRFProviderImpl>(),
        di::bind<extensions::ExtensionFactory>.template to<extensions::ExtensionFactoryImpl>(),
        di::bind<network::BabeGossiper>.template to<network::GossiperBroadcast>(),
        di::bind<consensus::grandpa::Gossiper>.template to<network::GossiperBroadcast>(),
        di::bind<network::Gossiper>.template to<network::GossiperBroadcast>(),
        di::bind<network::Router>.template to<network::RouterLibp2p>(),
        di::bind<network::SyncClientsSet>.to(std::move(get_sync_clients_set)),
        di::bind<network::SyncProtocolClient>.template to<consensus::SynchronizerImpl>(),
        di::bind<network::SyncProtocolObserver>.template to<consensus::SynchronizerImpl>(),
        di::bind<runtime::TaggedTransactionQueue>.template to<runtime::binaryen::TaggedTransactionQueueImpl>(),
        di::bind<runtime::ParachainHost>.template to<runtime::binaryen::ParachainHostImpl>(),
        di::bind<runtime::OffchainWorker>.template to<runtime::binaryen::OffchainWorkerImpl>(),
        di::bind<runtime::Metadata>.template to<runtime::binaryen::MetadataImpl>(),
        di::bind<runtime::Grandpa>.template to<runtime::binaryen::GrandpaImpl>(),
        di::bind<runtime::Core>.template to<runtime::binaryen::CoreImpl>(),
        di::bind<runtime::BabeApi>.template to<runtime::binaryen::BabeApiImpl>(),
        di::bind<runtime::BlockBuilder>.template to<runtime::binaryen::BlockBuilderImpl>(),
        di::bind<transaction_pool::TransactionPool>.template to<transaction_pool::TransactionPoolImpl>(),
        di::bind<transaction_pool::PoolModerator>.template to<transaction_pool::PoolModeratorImpl>(),
        di::bind<storage::trie::TrieStorageBackend>.to(
            std::move(get_polkadot_trie_db_backend)),
        di::bind<storage::trie::PolkadotTrieDb>.to(
            std::move(get_polkadot_trie_db)),
        di::bind<storage::trie::TrieDb>.to(std::move(get_trie_db_overlay)),
        di::bind<storage::trie::TrieDbReader>.to(std::move(get_trie_db_overlay)),
        di::bind<storage::trie::TrieDbFactory>.template to<storage::trie::InMemoryTrieDbFactory>(),
        di::bind<storage::trie::Codec>.template to<storage::trie::PolkadotCodec>(),
        di::bind<storage::trie_db_overlay::TrieDbOverlay>.to(std::move(get_trie_db_overlay)),
        di::bind<runtime::WasmProvider>.template to<runtime::StorageWasmProvider>(),
        di::bind<application::ConfigurationStorage>.to(
            [genesis_path](const auto &injector) {
              return get_configuration_storage(genesis_path, injector);
            }),
        di::bind<application::KeyStorage>.to(
            [keystore_path](const auto &injector) {
              return get_key_storage(keystore_path, injector);
            }),

        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
    auto leveldb_options = leveldb::Options();
    leveldb_options.create_if_missing = true;
  }

}  // namespace kagome::injector

#endif  // KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP
