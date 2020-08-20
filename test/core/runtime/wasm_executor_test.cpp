/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/wasm_executor.hpp"

#include <binaryen/shell-interface.h>
#include <gtest/gtest.h>

#include <boost/filesystem.hpp>
#include <fstream>

#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/crypto_store/crypto_store_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "extensions/impl/extension_factory_impl.hpp"
#include "mock/core/storage/changes_trie/changes_tracker_mock.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "runtime/binaryen/module/wasm_module_factory_impl.hpp"
#include "runtime/binaryen/runtime_manager.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/binaryen/runtime_api/core_factory_impl.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/impl/trie_storage_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "testutil/outcome.hpp"
#include "testutil/runtime/common/basic_wasm_provider.hpp"

using kagome::api::Session;
using kagome::common::Buffer;
using kagome::crypto::Bip39ProviderImpl;
using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::CryptoStoreImpl;
using kagome::crypto::ED25519ProviderImpl;
using kagome::crypto::HasherImpl;
using kagome::crypto::Pbkdf2ProviderImpl;
using kagome::crypto::Secp256k1ProviderImpl;
using kagome::crypto::SR25519ProviderImpl;
using kagome::primitives::BlockHash;
using kagome::runtime::TrieStorageProvider;
using kagome::runtime::TrieStorageProviderImpl;
using kagome::runtime::WasmProvider;
using kagome::runtime::binaryen::CoreFactoryImpl;
using kagome::runtime::binaryen::RuntimeManager;
using kagome::runtime::binaryen::WasmExecutor;
using kagome::storage::changes_trie::ChangesTrackerMock;
using kagome::storage::trie::PolkadotCodec;
using kagome::storage::trie::PolkadotTrieFactoryImpl;
using kagome::storage::trie::PolkadotTrieImpl;
using kagome::storage::trie::TrieSerializerImpl;
using kagome::storage::trie::TrieStorage;
using kagome::storage::trie::TrieStorageImpl;
using kagome::subscription::SubscriptionEngine;

namespace fs = boost::filesystem;

class WasmExecutorTest : public ::testing::Test {
 public:
  void SetUp() override {
    // path to a file with wasm code in wasm/ subfolder
    auto wasm_path =
        fs::path(__FILE__).parent_path().string() + "/wasm/sumtwo.wasm";
    wasm_provider_ =
        std::make_shared<kagome::runtime::BasicWasmProvider>(wasm_path);

    auto backend =
        std::make_shared<kagome::storage::trie::TrieStorageBackendImpl>(
            std::make_shared<kagome::storage::InMemoryStorage>(),
            kagome::common::Buffer{});

    auto trie_factory = std::make_shared<PolkadotTrieFactoryImpl>();
    auto codec = std::make_shared<PolkadotCodec>();
    auto serializer =
        std::make_shared<TrieSerializerImpl>(trie_factory, codec, backend);

    using SessionPtr = std::shared_ptr<Session>;
    using SubscriptionEngineType =
        SubscriptionEngine<Buffer, SessionPtr, Buffer, BlockHash>;
    auto trieDb = kagome::storage::trie::TrieStorageImpl::createEmpty(
                      trie_factory, codec, serializer, boost::none)
                      .value();

    storage_provider_ =
        std::make_shared<TrieStorageProviderImpl>(std::move(trieDb));

    auto random_generator = std::make_shared<BoostRandomGenerator>();
    auto sr25519_provider =
        std::make_shared<SR25519ProviderImpl>(random_generator);
    auto ed25519_provider = std::make_shared<ED25519ProviderImpl>();
    auto secp256k1_provider = std::make_shared<Secp256k1ProviderImpl>();
    auto hasher = std::make_shared<HasherImpl>();
    auto pbkdf2_provider = std::make_shared<Pbkdf2ProviderImpl>();
    auto bip39_provider = std::make_shared<Bip39ProviderImpl>(pbkdf2_provider);
    auto crypto_store = std::make_shared<CryptoStoreImpl>(ed25519_provider,
                                                          sr25519_provider,
                                                          secp256k1_provider,
                                                          bip39_provider,
                                                          random_generator);
    auto changes_tracker =
        std::make_shared<kagome::storage::changes_trie::ChangesTrackerMock>();
    auto extension_factory =
        std::make_shared<kagome::extensions::ExtensionFactoryImpl>(
            std::make_shared<ChangesTrackerMock>(),
            sr25519_provider,
            ed25519_provider,
            secp256k1_provider,
            hasher,
            crypto_store,
            bip39_provider, [this, changes_tracker](
    std::shared_ptr<kagome::runtime::WasmProvider> wasm_provider) {
      kagome::runtime::binaryen::CoreFactoryImpl factory(
          runtime_manager_,
          changes_tracker,
          std::make_shared<
              kagome::blockchain::BlockHeaderRepositoryMock>());
      return factory.createWithCode(std::move(wasm_provider));
    });

    auto module_factory =
        std::make_shared<kagome::runtime::binaryen::WasmModuleFactoryImpl>();

    runtime_manager_ =
        std::make_shared<RuntimeManager>(std::move(extension_factory),
                                         std::move(module_factory),
                                         storage_provider_,
                                         std::move(hasher));

    executor_ = std::make_shared<WasmExecutor>();
  }

 protected:
  std::shared_ptr<WasmExecutor> executor_;
  std::shared_ptr<RuntimeManager> runtime_manager_;
  std::shared_ptr<TrieStorageProvider> storage_provider_;
  std::shared_ptr<WasmProvider> wasm_provider_;
};

/**
 * @given wasm executor
 * @when call is invoked with wasm code with addTwo function
 * @then proper result is returned
 */
TEST_F(WasmExecutorTest, ExecuteCode) {
  EXPECT_OUTCOME_TRUE(environment,
                      runtime_manager_->createEphemeralRuntimeEnvironment(
                          wasm_provider_->getStateCode()));
  auto &&[module, memory, opt_batch] = std::move(environment);

  auto res = executor_->call(
      *module, "addTwo", wasm::LiteralList{wasm::Literal(1), wasm::Literal(2)});

  ASSERT_TRUE(res) << res.error().message();
  ASSERT_EQ(res.value().geti32(), 3);
}
