/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kagome/application/impl/app_configuration_impl.hpp>
#include <kagome/application/impl/app_state_manager_impl.hpp>
#include <kagome/application/impl/chain_spec_impl.hpp>
#include <kagome/blockchain/impl/block_storage_impl.hpp>
#include <kagome/crypto/bandersnatch/bandersnatch_provider_impl.hpp>
#include <kagome/crypto/bip39/impl/bip39_provider_impl.hpp>
#include <kagome/crypto/ecdsa/ecdsa_provider_impl.hpp>
#include <kagome/crypto/ed25519/ed25519_provider_impl.hpp>
#include <kagome/crypto/elliptic_curves/elliptic_curves_impl.hpp>
#include <kagome/crypto/hasher/hasher_impl.hpp>
#include <kagome/crypto/key_store/key_store_impl.hpp>
#include <kagome/crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp>
#include <kagome/crypto/sr25519/sr25519_provider_impl.hpp>
#include <kagome/host_api/impl/host_api_factory_impl.hpp>
#include <kagome/log/configurator.hpp>
#include <kagome/offchain/impl/offchain_persistent_storage.hpp>
#include <kagome/offchain/impl/offchain_worker_pool_impl.hpp>
#include <kagome/runtime/binaryen/instance_environment_factory.hpp>
#include <kagome/runtime/binaryen/module/module_factory_impl.hpp>
#include <kagome/runtime/common/core_api_factory_impl.hpp>
#include <kagome/runtime/common/module_repository_impl.hpp>
#include <kagome/runtime/common/runtime_instances_pool.hpp>
#include <kagome/runtime/common/runtime_properties_cache_impl.hpp>
#include <kagome/runtime/common/runtime_upgrade_tracker_impl.hpp>
#include <kagome/runtime/common/storage_code_provider.hpp>
#include <kagome/runtime/executor.hpp>
#include <kagome/runtime/module.hpp>
#include <kagome/runtime/wabt/instrument.hpp>
#include <kagome/storage/rocksdb/rocksdb.hpp>
#include <kagome/storage/trie/impl/trie_storage_backend_impl.hpp>
#include <kagome/storage/trie/impl/trie_storage_impl.hpp>
#include <kagome/storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp>
#include <kagome/storage/trie/serialization/trie_serializer_impl.hpp>
#include <kagome/storage/trie_pruner/impl/trie_pruner_impl.hpp>
#include <libp2p/common/final_action.hpp>
#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include <libp2p/log/configurator.hpp>

int main() {
  libp2p::common::FinalAction flush_std_streams_at_exit([] {
    std::cout.flush();
    std::cerr.flush();
  });

  auto logging_system = std::make_shared<soralog::LoggingSystem>(
      std::make_shared<kagome::log::Configurator>(
          std::make_shared<libp2p::log::Configurator>()));
  auto res = logging_system->configure();
  if (not res.message.empty()) {
    (res.has_error ? std::cerr : std::cout) << res.message << '\n';
  }
  if (res.has_error) {
    exit(EXIT_FAILURE);
  }
  kagome::log::setLoggingSystem(logging_system);

  kagome::application::AppConfigurationImpl app_config;

  rocksdb::Options db_options{};
  db_options.create_if_missing = true;
  std::shared_ptr<kagome::storage::RocksDb> database =
      kagome::storage::RocksDb::create("/tmp/kagome_tmp_db", db_options)
          .value();
  auto hasher = std::make_shared<kagome::crypto::HasherImpl>();
  auto header_repo =
      std::make_shared<kagome::blockchain::BlockHeaderRepositoryImpl>(database,
                                                                      hasher);

  using std::string_literals::operator""s;

  auto chain_spec = kagome::application::ChainSpecImpl::loadFrom(
                        kagome::filesystem::path(__FILE__).parent_path()
                        / "../../../examples/polkadot/polkadot.json"s)
                        .value();

  auto code_substitutes = chain_spec->codeSubstitutes();

  auto config = std::make_shared<kagome::application::AppConfigurationImpl>();

  auto trie_factory =
      std::make_shared<kagome::storage::trie::PolkadotTrieFactoryImpl>();
  auto codec = std::make_shared<kagome::storage::trie::PolkadotCodec>();
  auto node_storage_backend =
      std::make_shared<kagome::storage::trie::TrieStorageBackendImpl>(database);
  auto serializer = std::make_shared<kagome::storage::trie::TrieSerializerImpl>(
      trie_factory, codec, node_storage_backend);

  auto app_state_manager =
      std::make_shared<kagome::application::AppStateManagerImpl>();

  auto state_pruner =
      std::make_shared<kagome::storage::trie_pruner::TriePrunerImpl>(
          app_state_manager,
          node_storage_backend,
          serializer,
          codec,
          database,
          hasher,
          config);

  std::shared_ptr<kagome::storage::trie::TrieStorageImpl> trie_storage =
      kagome::storage::trie::TrieStorageImpl::createEmpty(
          trie_factory, codec, serializer, state_pruner)
          .value();

  auto batch =
      trie_storage
          ->getPersistentBatchAt(serializer->getEmptyRootHash(), std::nullopt)
          .value();
  auto root_hash =
      batch->commit(kagome::storage::trie::StateVersion::V0).value();
  auto block_storage =
      kagome::blockchain::BlockStorageImpl::create(root_hash, database, hasher)
          .value();
  std::shared_ptr<kagome::runtime::RuntimeUpgradeTracker>
      runtime_upgrade_tracker =
          std::move(kagome::runtime::RuntimeUpgradeTrackerImpl::create(
                        header_repo, database, code_substitutes, std::make_shared<kagome::blockchain::BlockTreeMock>())
                        .value());

  auto storage_batch =
      trie_storage
          ->getPersistentBatchAt(serializer->getEmptyRootHash(), std::nullopt)
          .value();
  for (auto &kv : chain_spec->getGenesisTopSection()) {
    storage_batch->put(kv.first, kv.second.view()).value();
  }
  storage_batch->commit(kagome::storage::trie::StateVersion::V0).value();

  auto code_provider =
      std::make_shared<const kagome::runtime::StorageCodeProvider>(
          trie_storage, runtime_upgrade_tracker, code_substitutes, chain_spec);

  auto generator =
      std::make_shared<libp2p::crypto::random::BoostRandomGenerator>();
  auto ecdsa_provider =
      std::make_shared<kagome::crypto::EcdsaProviderImpl>(hasher);
  auto ed25519_provider =
      std::make_shared<kagome::crypto::Ed25519ProviderImpl>(hasher);
  auto sr25519_provider =
      std::make_shared<kagome::crypto::Sr25519ProviderImpl>();
  auto bandersnatch_provider =
      std::make_shared<kagome::crypto::BandersnatchProviderImpl>(hasher);
  auto secp256k1_provider =
      std::make_shared<kagome::crypto::Secp256k1ProviderImpl>();
  auto pbkdf2_provider = std::make_shared<kagome::crypto::Pbkdf2ProviderImpl>();
  auto bip39_provider = std::make_shared<kagome::crypto::Bip39ProviderImpl>(
      pbkdf2_provider, hasher);

  auto elliptic_curves = std::make_shared<kagome::crypto::EllipticCurvesImpl>();

  auto key_store_dir = "/tmp/kagome_tmp_key_storage";
  std::shared_ptr<kagome::crypto::KeyFileStorage> key_fs =
      kagome::crypto::KeyFileStorage::createAt(key_store_dir).value();
  auto csprng =
      std::make_shared<libp2p::crypto::random::BoostRandomGenerator>();

  auto sr_suite = std::make_unique<
      kagome::crypto::KeySuiteStoreImpl<kagome::crypto::Sr25519Provider>>(
      sr25519_provider, bip39_provider, csprng, key_fs);
  auto ed_suite = std::make_unique<
      kagome::crypto::KeySuiteStoreImpl<kagome::crypto::Ed25519Provider>>(
      ed25519_provider, bip39_provider, csprng, key_fs);
  auto ecdsa_suite = std::make_unique<
      kagome::crypto::KeySuiteStoreImpl<kagome::crypto::EcdsaProvider>>(
      ecdsa_provider, bip39_provider, csprng, key_fs);
  auto bandersnatch_suite = std::make_unique<
      kagome::crypto::KeySuiteStoreImpl<kagome::crypto::BandersnatchProvider>>(
      bandersnatch_provider, bip39_provider, csprng, key_fs);

  auto crypto_store = std::make_shared<kagome::crypto::KeyStore>(
      std::move(sr_suite),
      std::move(ed_suite),
      std::move(ecdsa_suite),
      std::move(bandersnatch_suite),
      ed25519_provider,
      app_state_manager,
      kagome::crypto::KeyStore::Config{key_store_dir});

  auto offchain_persistent_storage =
      std::make_shared<kagome::offchain::OffchainPersistentStorageImpl>(
          database);

  auto offchain_worker_pool =
      std::make_shared<kagome::offchain::OffchainWorkerPoolImpl>();

  auto host_api_factory =
      std::make_shared<kagome::host_api::HostApiFactoryImpl>(
          kagome::host_api::OffchainExtensionConfig{},
          ecdsa_provider,
          ed25519_provider,
          sr25519_provider,
          bandersnatch_provider,
          secp256k1_provider,
          elliptic_curves,
          hasher,
          crypto_store,
          offchain_persistent_storage,
          offchain_worker_pool);

  auto cache = std::make_shared<kagome::runtime::RuntimePropertiesCacheImpl>();

  std::shared_ptr<kagome::runtime::RuntimeInstancesPoolImpl>
      runtime_instances_pool;
  auto injector = boost::di::make_injector(
      boost::di::bind<kagome::runtime::RuntimeInstancesPool>().to(
          [&](const auto &) { return runtime_instances_pool; }));
  auto core_factory = std::make_shared<kagome::runtime::CoreApiFactoryImpl>(
      hasher,
      injector
          .create<kagome::LazySPtr<kagome::runtime::RuntimeInstancesPool>>());

  auto instance_env_factory =
      std::make_shared<kagome::runtime::binaryen::InstanceEnvironmentFactory>(
          trie_storage, serializer, core_factory, host_api_factory);

  auto module_factory =
      std::make_shared<kagome::runtime::binaryen::ModuleFactoryImpl>(
          instance_env_factory, trie_storage, hasher);

  runtime_instances_pool =
      std::make_shared<kagome::runtime::RuntimeInstancesPoolImpl>(
          app_config,
          module_factory,
          std::make_shared<kagome::runtime::WasmInstrumenter>());
  auto module_repo = std::make_shared<kagome::runtime::ModuleRepositoryImpl>(
      runtime_instances_pool,
      hasher,
      header_repo,
      runtime_upgrade_tracker,
      trie_storage,
      module_factory,
      code_provider);

  [[maybe_unused]] auto ctx_factory =
      std::make_shared<kagome::runtime::RuntimeContextFactoryImpl>(module_repo,
                                                                   header_repo);
  [[maybe_unused]] auto executor =
      kagome::runtime::Executor(ctx_factory, cache);
  return 0;
}
