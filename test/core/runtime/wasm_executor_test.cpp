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
#include "runtime/binaryen/runtime_manager.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/trie/impl/polkadot_trie_db.hpp"
#include "storage/trie/impl/trie_db_backend_impl.hpp"
#include "testutil/outcome.hpp"
#include "testutil/runtime/common/basic_wasm_provider.hpp"

using kagome::common::Buffer;
using kagome::runtime::binaryen::RuntimeManager;
using kagome::runtime::binaryen::WasmExecutor;

namespace fs = boost::filesystem;

class WasmExecutorTest : public ::testing::Test {
 public:
  void SetUp() override {
    // path to a file with wasm code in wasm/ subfolder
    auto wasm_path =
        fs::path(__FILE__).parent_path().string() + "/wasm/sumtwo.wasm";
    auto wasm_provider =
        std::make_shared<kagome::runtime::BasicWasmProvider>(wasm_path);

    auto trieDb = kagome::storage::trie::PolkadotTrieDb::createEmpty(
        std::make_shared<kagome::storage::trie::TrieDbBackendImpl>(
            std::make_shared<kagome::storage::InMemoryStorage>(),
            kagome::common::Buffer{}));
    auto extencion_factory =
        std::make_shared<kagome::extensions::ExtensionFactoryImpl>(
            std::shared_ptr<kagome::storage::trie::TrieDb>(trieDb.release()));

    auto hasher =
        std::make_shared<kagome::crypto::HasherImpl>();

    runtime_manager_ = std::make_shared<RuntimeManager>(
        std::move(wasm_provider), std::move(extencion_factory), std::move(hasher));

    executor_ = std::make_shared<WasmExecutor>();
  }

 protected:
  std::shared_ptr<WasmExecutor> executor_;
  std::shared_ptr<RuntimeManager> runtime_manager_;
};

/**
 * @given wasm executor
 * @when call is invoked with wasm code with addTwo function
 * @then proper result is returned
 */
TEST_F(WasmExecutorTest, ExecuteCode) {
  EXPECT_OUTCOME_TRUE(environment, runtime_manager_->getRuntimeEvironment());
  auto &&[module, memory] = std::move(environment);

  auto res = executor_->call(
      *module, "addTwo", wasm::LiteralList{wasm::Literal(1), wasm::Literal(2)});

  ASSERT_TRUE(res) << res.error().message();
  ASSERT_EQ(res.value().geti32(), 3);
}
