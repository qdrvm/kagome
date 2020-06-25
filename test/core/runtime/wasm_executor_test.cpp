/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/wasm_executor.hpp"

#include <binaryen/shell-interface.h>
#include <gtest/gtest.h>

#include <boost/filesystem.hpp>
#include <fstream>

#include "crypto/hasher/hasher_impl.hpp"
#include "extensions/impl/extension_factory_impl.hpp"
#include "mock/core/storage/changes_trie/changes_tracker_mock.hpp"
#include "runtime/binaryen/runtime_manager.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/impl/trie_storage_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "testutil/outcome.hpp"
#include "testutil/runtime/common/basic_wasm_provider.hpp"

using kagome::common::Buffer;
using kagome::runtime::TrieStorageProvider;
using kagome::runtime::TrieStorageProviderImpl;
using kagome::runtime::binaryen::RuntimeManager;
using kagome::runtime::binaryen::WasmExecutor;
using kagome::storage::changes_trie::ChangesTrackerMock;
using kagome::storage::trie::PolkadotCodec;
using kagome::storage::trie::PolkadotTrieFactoryImpl;
using kagome::storage::trie::PolkadotTrieImpl;
using kagome::storage::trie::TrieSerializerImpl;
using kagome::storage::trie::TrieStorage;
using kagome::storage::trie::TrieStorageImpl;

namespace fs = boost::filesystem;

class WasmExecutorTest : public ::testing::Test {
 public:
  void SetUp() override {
    // path to a file with wasm code in wasm/ subfolder
    auto wasm_path =
        fs::path(__FILE__).parent_path().string() + "/wasm/sumtwo.wasm";
    auto wasm_provider =
        std::make_shared<kagome::runtime::BasicWasmProvider>(wasm_path);

    auto backend =
        std::make_shared<kagome::storage::trie::TrieStorageBackendImpl>(
            std::make_shared<kagome::storage::InMemoryStorage>(),
            kagome::common::Buffer{});

    auto trie_factory = std::make_shared<PolkadotTrieFactoryImpl>();
    auto codec = std::make_shared<PolkadotCodec>();
    auto serializer =
        std::make_shared<TrieSerializerImpl>(trie_factory, codec, backend);

    auto trieDb = kagome::storage::trie::TrieStorageImpl::createEmpty(
                      trie_factory, codec, serializer, boost::none)
                      .value();

    storage_provider_ =
        std::make_shared<TrieStorageProviderImpl>(std::move(trieDb));

    auto extension_factory =
        std::make_shared<kagome::extensions::ExtensionFactoryImpl>(
            std::make_shared<ChangesTrackerMock>());

    auto hasher = std::make_shared<kagome::crypto::HasherImpl>();

    runtime_manager_ =
        std::make_shared<RuntimeManager>(std::move(wasm_provider),
                                         std::move(extension_factory),
                                         storage_provider_,
                                         std::move(hasher));

    executor_ = std::make_shared<WasmExecutor>();
  }

 protected:
  std::shared_ptr<WasmExecutor> executor_;
  std::shared_ptr<RuntimeManager> runtime_manager_;
  std::shared_ptr<TrieStorageProvider> storage_provider_;
};

/**
 * @given wasm executor
 * @when call is invoked with wasm code with addTwo function
 * @then proper result is returned
 */
TEST_F(WasmExecutorTest, ExecuteCode) {
  EXPECT_OUTCOME_TRUE(environment,
                      runtime_manager_->createEphemeralRuntimeEnvironment());
  auto &&[module, memory] = std::move(environment);

  auto res = executor_->call(
      *module, "addTwo", wasm::LiteralList{wasm::Literal(1), wasm::Literal(2)});

  ASSERT_TRUE(res) << res.error().message();
  ASSERT_EQ(res.value().geti32(), 3);
}
