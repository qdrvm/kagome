/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <filesystem>

#include <kagome/application/impl/chain_spec_impl.hpp>
#include <kagome/blockchain/impl/block_header_repository_impl.hpp>
#include <kagome/crypto/bip39/impl/bip39_provider_impl.hpp>
#include <kagome/crypto/crypto_store/crypto_store_impl.hpp>
#include <kagome/crypto/ecdsa/ecdsa_provider_impl.hpp>
#include <kagome/crypto/ed25519/ed25519_provider_impl.hpp>
#include <kagome/crypto/hasher/hasher_impl.hpp>
#include <kagome/crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp>
#include <kagome/crypto/secp256k1/secp256k1_provider_impl.hpp>
#include <kagome/crypto/sr25519/sr25519_provider_impl.hpp>
#include <kagome/host_api/impl/host_api_factory_impl.hpp>
#include <kagome/log/configurator.hpp>
#include <kagome/offchain/impl/offchain_persistent_storage.hpp>
#include <kagome/offchain/impl/offchain_worker_pool_impl.hpp>
#include <kagome/runtime/common/module_repository_impl.hpp>
#include <kagome/runtime/common/runtime_upgrade_tracker_impl.hpp>
#include <kagome/runtime/common/storage_code_provider.hpp>
#include <kagome/runtime/executor.hpp>
#include <kagome/runtime/wavm/compartment_wrapper.hpp>
#include <kagome/runtime/wavm/instance_environment_factory.hpp>
#include <kagome/runtime/wavm/intrinsics/intrinsic_module.hpp>
#include <kagome/runtime/wavm/module_factory_impl.hpp>
#include <kagome/storage/changes_trie/impl/storage_changes_tracker_impl.hpp>
#include <kagome/storage/leveldb/leveldb.hpp>
#include <kagome/storage/trie/impl/trie_storage_backend_impl.hpp>
#include <kagome/storage/trie/impl/trie_storage_impl.hpp>
#include <kagome/storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp>
#include <kagome/storage/trie/serialization/polkadot_codec.hpp>
#include <kagome/storage/trie/serialization/trie_serializer_impl.hpp>
#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include <libp2p/log/configurator.hpp>

kagome::storage::trie::RootHash trieRoot(
    const std::vector<std::pair<kagome::common::Buffer, kagome::common::Buffer>>
        &key_vals) {
  auto trie = kagome::storage::trie::PolkadotTrieImpl();
  auto codec = kagome::storage::trie::PolkadotCodec();

  for (const auto &[key, val] : key_vals) {
    [[maybe_unused]] auto res = trie.put(key, val);
    BOOST_ASSERT_MSG(res.has_value(), "Insertion into trie failed");
  }
  auto root = trie.getRoot();
  if (root == nullptr) {
    return codec.hash256({0});
  }
  auto encode_res = codec.encodeNode(*root);
  BOOST_ASSERT_MSG(encode_res.has_value(), "Trie encoding failed");
  return codec.hash256(encode_res.value());
}

int main() {
  auto logging_system = std::make_shared<soralog::LoggingSystem>(
      std::make_shared<kagome::log::Configurator>(
          std::make_shared<libp2p::log::Configurator>()));
  auto res = logging_system->configure();
  if (not res.message.empty()) {
    (res.has_error ? std::cerr : std::cout) << res.message << std::endl;
  }
  if (res.has_error) {
    exit(EXIT_FAILURE);
  }
  kagome::log::setLoggingSystem(logging_system);

  leveldb::Options db_options{};
  db_options.create_if_missing = true;
  std::shared_ptr database =
      kagome::storage::LevelDB::create("/tmp/kagome_tmp_db", db_options)
          .value();
  auto hasher = std::make_shared<kagome::crypto::HasherImpl>();
  auto header_repo =
      std::make_shared<kagome::blockchain::BlockHeaderRepositoryImpl>(database,
                                                                      hasher);

  using std::string_literals::operator""s;

  auto chain_spec = kagome::application::ChainSpecImpl::loadFrom(
                        std::filesystem::path(__FILE__).parent_path()
                        / "../../../examples/polkadot/polkadot.json"s)
                        .value();

  auto code_substitutes = chain_spec->codeSubstitutes();
  auto runtime_upgrade_tracker =
      std::shared_ptr(kagome::runtime::RuntimeUpgradeTrackerImpl::create(
                          header_repo, database, code_substitutes)
                          .value());

  auto trie_factory =
      std::make_shared<kagome::storage::trie::PolkadotTrieFactoryImpl>();
  auto codec = std::make_shared<kagome::storage::trie::PolkadotCodec>();
  auto storage_backend =
      std::make_shared<kagome::storage::trie::TrieStorageBackendImpl>(
          database, kagome::common::Buffer::fromHex("DEADBEEF").value());
  auto serializer = std::make_shared<kagome::storage::trie::TrieSerializerImpl>(
      trie_factory, codec, storage_backend);
  auto storage_subscription_engine =
      std::make_shared<kagome::primitives::events::StorageSubscriptionEngine>();
  auto chain_subscription_engine =
      std::make_shared<kagome::primitives::events::ChainSubscriptionEngine>();

  auto changes_tracker = std::make_shared<
      kagome::storage::changes_trie::StorageChangesTrackerImpl>(
      trie_factory,
      codec,
      storage_subscription_engine,
      chain_subscription_engine);

  auto trie_storage =
      std::shared_ptr(kagome::storage::trie::TrieStorageImpl::createEmpty(
                          trie_factory, codec, serializer, changes_tracker)
                          .value());
  auto storage_batch =
      trie_storage->getPersistentBatchAt(serializer->getEmptyRootHash())
          .value();
  for (auto &kv : chain_spec->getGenesis()) {
    storage_batch->put(kv.first, kv.second).value();
  }
  storage_batch->commit().value();

  auto code_provider =
      std::make_shared<const kagome::runtime::StorageCodeProvider>(
          trie_storage, runtime_upgrade_tracker, code_substitutes, chain_spec);
  auto compartment =
      std::make_shared<kagome::runtime::wavm::CompartmentWrapper>(
          "WAVM Compartment");
  auto intrinsic_module =
      std::make_shared<const kagome::runtime::wavm::IntrinsicModule>(
          compartment);

  auto generator =
      std::make_shared<libp2p::crypto::random::BoostRandomGenerator>();
  auto sr25519_provider =
      std::make_shared<kagome::crypto::Sr25519ProviderImpl>(generator);
  auto ecdsa_provider = std::make_shared<kagome::crypto::EcdsaProviderImpl>();
  auto ed25519_provider =
      std::make_shared<kagome::crypto::Ed25519ProviderImpl>(generator);
  auto secp256k1_provider =
      std::make_shared<kagome::crypto::Secp256k1ProviderImpl>();
  auto pbkdf2_provider = std::make_shared<kagome::crypto::Pbkdf2ProviderImpl>();
  auto bip39_provider =
      std::make_shared<kagome::crypto::Bip39ProviderImpl>(pbkdf2_provider);

  auto ecdsa_suite =
      std::make_shared<kagome::crypto::EcdsaSuite>(ecdsa_provider);
  auto ed_suite =
      std::make_shared<kagome::crypto::Ed25519Suite>(ed25519_provider);
  auto sr_suite =
      std::make_shared<kagome::crypto::Sr25519Suite>(sr25519_provider);
  auto key_fs = std::shared_ptr(
      kagome::crypto::KeyFileStorage::createAt("/tmp/kagome_tmp_key_storage")
          .value());
  auto crypto_store = std::make_shared<kagome::crypto::CryptoStoreImpl>(
      ecdsa_suite, ed_suite, sr_suite, bip39_provider, key_fs);

  auto offchain_persistent_storage =
      std::make_shared<kagome::offchain::OffchainPersistentStorageImpl>(
          database);

  auto offchain_worker_pool =
      std::make_shared<kagome::offchain::OffchainWorkerPoolImpl>();

  auto host_api_factory =
      std::make_shared<kagome::host_api::HostApiFactoryImpl>(
          kagome::host_api::OffchainExtensionConfig{},
          changes_tracker,
          sr25519_provider,
          ecdsa_provider,
          ed25519_provider,
          secp256k1_provider,
          hasher,
          crypto_store,
          bip39_provider,
          offchain_persistent_storage,
          offchain_worker_pool);

  auto instance_env_factory =
      std::make_shared<const kagome::runtime::wavm::InstanceEnvironmentFactory>(
          trie_storage,
          serializer,
          compartment,
          intrinsic_module,
          host_api_factory,
          header_repo,
          changes_tracker);
  auto module_factory =
      std::make_shared<kagome::runtime::wavm::ModuleFactoryImpl>(
          compartment, instance_env_factory, intrinsic_module);
  auto module_repo = std::make_shared<kagome::runtime::ModuleRepositoryImpl>(
      runtime_upgrade_tracker, module_factory);
  auto env_factory =
      std::make_shared<kagome::runtime::RuntimeEnvironmentFactory>(
          code_provider, module_repo, header_repo);

  [[maybe_unused]] auto executor =
      kagome::runtime::Executor(header_repo, env_factory);

  // TODO(Harrm): Currently, the test only checks if kagome builds as
  // a dependency in some project. However, we can use the test to run
  // some integration tests, like it's done in polkadot tests

  return 0;
}
