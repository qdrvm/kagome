/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "injector/application_injector.hpp"

namespace kagome::injector {

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

    initialized = std::make_shared<api::HttpListenerImpl>(
        app_state_manager, context, listener_config, http_session_config);
    return initialized.value();
  }

  sptr<api::WsListenerImpl> get_jrpc_api_ws_listener(
      api::WsSession::Configuration ws_session_config,
      sptr<api::RpcContext> context,
      sptr<application::AppStateManager> app_state_manager,
      const boost::asio::ip::tcp::endpoint &endpoint) {
    static auto initialized =
        boost::optional<sptr<api::WsListenerImpl>>(boost::none);
    if (initialized) {
      return initialized.value();
    }

    api::WsListenerImpl::Configuration listener_config;
    listener_config.endpoint = endpoint;

    initialized =
        std::make_shared<api::WsListenerImpl>(app_state_manager,
                                              context,
                                              listener_config,
                                              std::move(ws_session_config));
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

    auto storage = blockchain::KeyValueBlockStorage::create(
        trie_storage->getRootHash(),
        db,
        hasher,
        [&db, &grandpa_api](const primitives::Block &genesis_block) {
          // handle genesis initialization, which happens when there is not
          // authorities and last completed round in the storage
          if (not db->get(storage::kAuthoritySetKey)) {
            // insert authorities
            const auto &weighted_authorities_res = grandpa_api->authorities(
                primitives::BlockId(primitives::BlockNumber{0}));
            BOOST_ASSERT_MSG(weighted_authorities_res,
                             "grandpa_api_->authorities failed");
            const auto &weighted_authorities = weighted_authorities_res.value();

            for (const auto &authority : weighted_authorities) {
              spdlog::info("Grandpa authority: {}", authority.id.id.toHex());
            }

            consensus::grandpa::VoterSet voters{0};
            for (const auto &weighted_authority : weighted_authorities) {
              voters.insert(
                  primitives::GrandpaSessionKey{weighted_authority.id.id},
                  weighted_authority.weight);
              spdlog::debug("Added to grandpa authorities: {}, weight: {}",
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

  sptr<storage::trie::TrieStorageBackendImpl> get_trie_storage_backend(
      sptr<storage::BufferStorage> storage) {
    static auto initialized =
        boost::optional<sptr<storage::trie::TrieStorageBackendImpl>>(
            boost::none);

    if (initialized) {
      return initialized.value();
    }
    using blockchain::prefix::TRIE_NODE;
    auto backend = std::make_shared<storage::trie::TrieStorageBackendImpl>(
        storage, common::Buffer{TRIE_NODE});
    initialized = backend;
    return backend;
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

    sptr<storage::trie::TrieStorageImpl> trie_storage =
        std::move(trie_storage_res.value());
    initialized = trie_storage;
    return trie_storage;
  }

  sptr<storage::trie::TrieStorage> get_trie_storage(
      sptr<application::ChainSpec> configuration_storage,
      sptr<storage::trie::TrieStorageImpl> trie_storage) {
    static auto initialized =
        boost::optional<sptr<storage::trie::TrieStorage>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    const auto &genesis_raw_configs = configuration_storage->getGenesis();

    auto batch = trie_storage->getPersistentBatch();
    if (not batch) {
      common::raise(batch.error());
    }
    for (const auto &[key, val] : genesis_raw_configs) {
      spdlog::debug(
          "Key: {}, Val: {}", key.toHex(), val.toHex().substr(0, 200));
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
    auto db = storage::LevelDB::create(
        app_config.databasePath(chain_spec->id()), options);
    if (!db) {
      spdlog::critical("Can't create LevelDB in {}: {}",
                       fs::absolute(app_config.databasePath(chain_spec->id()),
                                    fs::current_path())
                           .native(),
                       db.error().message());
      exit(EXIT_FAILURE);
    }
    initialized = db.value();
    return initialized.value();
  }

  std::shared_ptr<application::ChainSpec> get_genesis_config(
      application::AppConfiguration const &config) {
    static auto initialized =
        boost::optional<sptr<application::ChainSpec>>(boost::none);
    if (initialized) {
      return initialized.value();
    }
    auto const &genesis_path = config.genesisPath();

    auto genesis_config_res =
        application::ChainSpecImpl::loadFrom(genesis_path.native());
    if (genesis_config_res.has_error()) {
      common::raise(genesis_config_res.error());
    }
    initialized = genesis_config_res.value();
    return genesis_config_res.value();
  }

  sptr<primitives::BabeConfiguration> get_babe_configuration(
      sptr<runtime::BabeApi> babe_api) {
    static auto initialized =
        boost::optional<sptr<primitives::BabeConfiguration>>(boost::none);
    if (initialized) {
      return *initialized;
    }

    auto configuration_res = babe_api->configuration();
    if (not configuration_res) {
      common::raise(configuration_res.error());
    }
    auto config = configuration_res.value();
    for (const auto &authority : config.genesis_authorities) {
      spdlog::debug("Babe authority: {}", authority.id.id.toHex());
    }
    config.leadership_rate.first *= 3;
    initialized = std::make_shared<primitives::BabeConfiguration>(config);
    return *initialized;
  }

  consensus::SlotsStrategy get_slots_strategy(
      const application::AppConfiguration &app_config) {
    static auto initialized =
        boost::optional<consensus::SlotsStrategy>(boost::none);
    if (not initialized) {
      initialized = app_config.isUnixSlotsStrategy()
                        ? consensus::SlotsStrategy::FromUnixEpoch
                        : consensus::SlotsStrategy::FromZero;
    }
    return *initialized;
  }

  sptr<crypto::KeyFileStorage> get_key_file_storage(
      application::AppConfiguration const &config,
      sptr<application::ChainSpec> genesis_config) {
    static auto initialized =
        boost::optional<sptr<crypto::KeyFileStorage>>(boost::none);
    if (initialized) {
      return *initialized;
    }

    auto path = config.keystorePath(genesis_config->id());
    auto key_file_storage_res = crypto::KeyFileStorage::createAt(path);
    if (not key_file_storage_res) {
      common::raise(key_file_storage_res.error());
    }
    initialized = std::move(key_file_storage_res.value());

    return *initialized;
  }

  sptr<consensus::grandpa::FinalizationObserver>
  get_finalization_observer(
      sptr<authority::AuthorityManagerImpl> authority_manager) {
    static auto instance = boost::optional<
        std::shared_ptr<consensus::grandpa::FinalizationObserver>>(boost::none);
    if (instance) {
      return *instance;
    }

    instance = std::make_shared<consensus::grandpa::FinalizationComposite>(
        std::move(authority_manager));

    return *instance;
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

    if (app_config.nodeKey()) {
      spdlog::info("Will use LibP2P keypair from config or args");

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
      spdlog::info("Will use LibP2P keypair from key storage");

      auto stored_keypair = crypto_store.getLibp2pKeypair().value();

      initialized =
          std::make_shared<libp2p::crypto::KeyPair>(std::move(stored_keypair));
      return initialized.value();
    }

    spdlog::warn(
        "Can not get LibP2P keypair from crypto storage. "
        "Will be temporary generated unique one");

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

  sptr<libp2p::protocol::kademlia::Config> get_kademlia_config(
      const application::ChainSpec &chain_spec) {
    static auto initialized =
        boost::optional<sptr<libp2p::protocol::kademlia::Config>>(boost::none);
    if (initialized) {
      return *initialized;
    }

    auto kagome_config = std::make_shared<libp2p::protocol::kademlia::Config>(
        libp2p::protocol::kademlia::Config{
            .protocolId = "/" + chain_spec.protocolId() + "/kad",
            .maxBucketSize = 1000,
            .randomWalk = {.interval = std::chrono::minutes(1)}});

    initialized = std::move(kagome_config);
    return *initialized;
  }

}
