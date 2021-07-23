/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_external_interface.hpp"

#include <binaryen/wasm-s-parser.h>
#include <boost/format.hpp>

#include "crypto/crypto_store/key_type.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/host_api/host_api_factory_mock.hpp"
#include "mock/core/host_api/host_api_mock.hpp"
#include "mock/core/runtime/binaryen_wasm_memory_factory_mock.hpp"
#include "mock/core/runtime/core_api_provider_mock.hpp"
#include "mock/core/runtime/core_factory_mock.hpp"
#include "mock/core/runtime/memory_mock.hpp"
#include "mock/core/runtime/memory_provider_mock.hpp"
#include "mock/core/runtime/module_repository_mock.hpp"
#include "mock/core/runtime/runtime_environment_factory_mock.hpp"
#include "mock/core/runtime/trie_storage_provider_mock.hpp"
#include "runtime/common/constant_code_provider.hpp"
#include "runtime/common/memory_allocator.hpp"
#include "runtime/ptr_size.hpp"
#include "testutil/prepare_loggers.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::crypto::KEY_TYPE_BABE;
using kagome::host_api::HostApi;
using kagome::host_api::HostApiFactoryMock;
using kagome::host_api::HostApiMock;
using kagome::runtime::ConstantCodeProvider;
using kagome::runtime::CoreApiProviderMock;
using kagome::runtime::MemoryMock;
using kagome::runtime::MemoryProviderMock;
using kagome::runtime::ModuleRepositoryMock;
using kagome::runtime::PtrSize;
using kagome::runtime::RuntimeEnvironmentFactoryMock;
using kagome::runtime::TrieStorageProviderMock;
using kagome::runtime::WasmEnum;
using kagome::runtime::WasmLogLevel;
using kagome::runtime::WasmOffset;
using kagome::runtime::WasmPointer;
using kagome::runtime::WasmSize;
using kagome::runtime::WasmSpan;
using kagome::runtime::binaryen::BinaryenWasmMemoryFactoryMock;
using kagome::runtime::binaryen::CoreFactoryMock;
using kagome::runtime::binaryen::RuntimeExternalInterface;
using wasm::Element;
using wasm::Module;
using wasm::ModuleInstance;
using wasm::SExpressionParser;
using wasm::SExpressionWasmBuilder;

/// extend Runtime external interface by adding wasm assertion functions
class TestableExternalInterface : public RuntimeExternalInterface {
 public:
  using RuntimeExternalInterface::RuntimeExternalInterface;

  wasm::Literal callImport(wasm::Function *import,
                           wasm::LiteralList &arguments) override {
    if (import->module == "env" && import->base == "assert") {
      EXPECT_TRUE(arguments.at(0).geti32());
      return wasm::Literal();
    }
    if (import->module == "env" && import->base == "assert_eq_i32") {
      EXPECT_EQ(arguments.at(0).geti32(), arguments.at(1).geti32());
      return wasm::Literal();
    }
    if (import->module == "env" && import->base == "assert_eq_i64") {
      EXPECT_EQ(arguments.at(0).geti64(), arguments.at(1).geti64());
      return wasm::Literal();
    }
    return RuntimeExternalInterface::callImport(import, arguments);
  }
};

class REITest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    memory_ = std::make_shared<MemoryMock>();
    host_api_ = std::make_unique<HostApiMock>();
    host_api_factory_ = std::make_shared<HostApiFactoryMock>();
    storage_provider_ = std::make_shared<TrieStorageProviderMock>();
    core_api_provider_ = std::make_shared<CoreApiProviderMock>();
    memory_provider_ = std::make_shared<MemoryProviderMock>();
    auto code_provider =
        std::make_shared<ConstantCodeProvider>(kagome::common::Buffer{});
    auto module_repo = std::make_shared<ModuleRepositoryMock>();
    auto header_repo = std::make_shared<BlockHeaderRepositoryMock>();
    runtime_env_factory_ =
        std::make_shared<RuntimeEnvironmentFactoryMock>(storage_provider_,
                                                        host_api_,
                                                        memory_provider_,
                                                        code_provider,
                                                        module_repo,
                                                        header_repo);
  }

  void executeWasm(std::string call_code) {
    std::string code = (boost::format(wasm_template_) % call_code).str();

    // parse wast
    Module wasm{};

    // clang-8 doesn't know char * std::string::data(),
    // it returns only const char *
    char *data = const_cast<char *>(code.data());
    SExpressionParser parser(data);
    Element &root = *parser.root;
    ASSERT_GT(root.size(), 0);
    ASSERT_NE(root[0], nullptr);
    SExpressionWasmBuilder builder(wasm, *root[0]);
    EXPECT_CALL(*host_api_, memory()).WillRepeatedly(Return(memory_));

    TestableExternalInterface rei(host_api_);

    // interpret module
    ModuleInstance instance(wasm, &rei);
  }

 protected:
  std::shared_ptr<MemoryMock> memory_;
  std::shared_ptr<CoreApiProviderMock> core_api_provider_;
  std::shared_ptr<RuntimeEnvironmentFactoryMock> runtime_env_factory_;
  std::shared_ptr<HostApiMock> host_api_;
  std::shared_ptr<HostApiFactoryMock> host_api_factory_;
  std::shared_ptr<TrieStorageProviderMock> storage_provider_;
  std::shared_ptr<MemoryProviderMock> memory_provider_;

  // clang-format off
  const std::string wasm_template_ =
      "(module\n"
      "  (type (;0;) (func (param i32 i32)))\n"
      "  (type (;1;) (func (param i32)))\n"
      "  (type (;2;) (func (param i32 i32 i32) (result i32)))\n"
      "  (type (;3;) (func (param i32 i32) (result i32)))\n"
      "  (type (;4;) (func (param i32 i32 i32 i32 i32) (result i32)))\n"
      "  (type (;5;) (func (param i32 i32 i32)))\n"
      "  (type (;6;) (func (param i32 i32 i32 i32)))\n"
      "  (type (;7;) (func (param i64)))\n"
      "  (type (;8;) (func (param i32) (result i32)))\n"
      "  (type (;9;) (func (param i32 i32 i32 i32) (result i32)))\n"
      "  (type (;10;) (func (param i32 i32 i64 i32) (result i32)))\n"
      "  (type (;11;) (func))\n"
      "  (type (;12;) (func (param i32 i64 i64)))\n"
      "  (type (;13;) (func (param i64 i32) (result i32)))\n"
      "  (type (;14;) (func (param i32) (result i64)))\n"
      "  (type (;15;) (func (param i64 i32)))\n"
      "  (type (;16;) (func (param i32 i32 i64 i64)))\n"
      "  (type (;17;) (func (param i32 i64 i64 i64)))\n"
      "  (type (;18;) (func (param i64 i64)))\n"
      "  (type (;19;) (func (param i32 i32 i32 i32 i32)))\n"
      "  (type (;20;) (func (param i32 i64)))\n"
      "  (type (;21;) (func (param i32 i64 i32 i32 i64)))\n"
      "  (type (;22;) (func (param i32 i32 i32 i64 i64)))\n"
      "  (type (;23;) (func (param i32 i32) (result i64)))\n"
      "  (type (;24;) (func (param i32 i64 i64 i64 i64)))\n"
      "  (type (;25;) (func (param i32 i64 i64 i32)))\n"
      "  (type (;26;) (func (param i32 i64 i64 i64 i64 i32)))\n"
      "  (type (;27;) (func (result i64)))\n"
      "  (type (;28;) (func (param i32 i32 i32)))\n"
      "  (type (;29;) (func (param i64) (result i64)))\n"
      "  (type (;30;) (func (param i32 i64) (result i32)))\n"
      "  (type (;31;) (func (param i32 i32 i64) (result i64)))\n"
      "  (type (;32;) (func (param i32 i64 i32) (result i32)))\n"
      "  (type (;33;) (func (param i64 i64 i32) (result i64)))\n"
      "  (type (;34;) (func (param i64) (result i32)))\n"
      "  (type (;35;) (func (result i32)))\n"

      /// version 1
      "  (import \"env\" \"ext_crypto_start_batch_verify\" (func $ext_crypto_start_batch_verify_version_1 (type 11)))\n"
      "  (import \"env\" \"ext_crypto_finish_batch_verify\" (func $ext_crypto_finish_batch_verify_version_1 (type 35)))\n"
      "  (import \"env\" \"ext_crypto_ed25519_public_keys_version_1\" (func $ext_crypto_ed25519_public_keys_version_1 (type 29)))\n"
      "  (import \"env\" \"ext_crypto_ed25519_generate_version_1\" (func $ext_crypto_ed25519_generate_version_1 (type 30)))\n"
      "  (import \"env\" \"ext_crypto_ed25519_sign_version_1\" (func $ext_crypto_ed25519_sign_version_1 (type 31)))\n"
      "  (import \"env\" \"ext_crypto_ed25519_verify_version_1\" (func $ext_crypto_ed25519_verify_version_1 (type 32)))\n"
      "  (import \"env\" \"ext_crypto_sr25519_public_keys_version_1\" (func $ext_crypto_sr25519_public_keys_version_1 (type 29)))\n"
      "  (import \"env\" \"ext_crypto_sr25519_generate_version_1\" (func $ext_crypto_sr25519_generate_version_1 (type 30)))\n"
      "  (import \"env\" \"ext_crypto_sr25519_sign_version_1\" (func $ext_crypto_sr25519_sign_version_1 (type 31)))\n"
      "  (import \"env\" \"ext_crypto_sr25519_verify_version_2\" (func $ext_crypto_sr25519_verify_version_2 (type 32)))\n"
      "  (import \"env\" \"ext_crypto_secp256k1_ecdsa_recover_version_1\" (func $ext_crypto_secp256k1_ecdsa_recover_version_1 (type 31)))\n"
      "  (import \"env\" \"ext_crypto_secp256k1_ecdsa_recover_compressed_version_1\" (func $ext_crypto_secp256k1_ecdsa_recover_compressed_version_1 (type 31)))\n"

      /// hashing methods
      "  (import \"env\" \"ext_hashing_keccak_256_version_1\" (func $ext_hashing_keccak_256_version_1 (type 34)))\n"
      "  (import \"env\" \"ext_hashing_sha2_256_version_1\" (func $ext_hashing_sha2_256_version_1 (type 34)))\n"
      "  (import \"env\" \"ext_hashing_blake2_128_version_1\" (func $ext_hashing_blake2_128_version_1 (type 34)))\n"
      "  (import \"env\" \"ext_hashing_blake2_256_version_1\" (func $ext_hashing_blake2_256_version_1 (type 34)))\n"
      "  (import \"env\" \"ext_hashing_twox_256_version_1\" (func $ext_hashing_twox_256_version_1 (type 34)))\n"
      "  (import \"env\" \"ext_hashing_twox_128_version_1\" (func $ext_hashing_twox_128_version_1 (type 34)))\n"
      "  (import \"env\" \"ext_hashing_twox_64_version_1\" (func $ext_hashing_twox_64_version_1 (type 34)))\n"

      /// allocator methods
      "  (import \"env\" \"ext_allocator_malloc_version_1\" (func $ext_allocator_malloc_version_1 (type 8)))\n"
      "  (import \"env\" \"ext_allocator_free_version_1\" (func $ext_allocator_free_version_1 (type 1)))\n"

      /// storage methods
      "  (import \"env\" \"ext_storage_set_version_1\" (func $ext_storage_set_version_1 (type 18)))\n"
      "  (import \"env\" \"ext_storage_get_version_1\" (func $ext_storage_get_version_1 (type 29)))\n"
      "  (import \"env\" \"ext_storage_clear_version_1\" (func $ext_storage_clear_version_1 (type 7)))\n"
      "  (import \"env\" \"ext_storage_exists_version_1\" (func $ext_storage_exists_version_1 (type 34)))\n"
      "  (import \"env\" \"ext_storage_read_version_1\" (func $ext_storage_read_version_1 (type 33)))\n"
      "  (import \"env\" \"ext_storage_clear_prefix_version_1\" (func $ext_storage_clear_prefix_version_1 (type 7)))\n"
      "  (import \"env\" \"ext_storage_changes_root_version_1\" (func $ext_storage_changes_root_version_1 (type 34)))\n"
      "  (import \"env\" \"ext_storage_root_version_1\" (func $ext_storage_root_version_1 (type 35)))\n"
      "  (import \"env\" \"ext_storage_next_key_version_1\" (func $ext_storage_next_key_version_1 (type 29)))\n"

      /// trie methods
      "  (import \"env\" \"ext_trie_blake2_256_root_version_1\" (func $ext_trie_blake2_256_root_version_1 (type 34)))\n"
      "  (import \"env\" \"ext_trie_blake2_256_ordered_root_version_1\" (func $ext_trie_blake2_256_ordered_root_version_1 (type 34)))\n"

      /// assertions to check output in wasm
      "  (import \"env\" \"assert\" (func $assert (param i32)))\n"
      "  (import \"env\" \"assert_eq_i32\" (func $assert_eq_i32 (param i32 i32)))\n"
      "  (import \"env\" \"assert_eq_i64\" (func $assert_eq_i64 (param i64 i64)))\n"

      "  (import \"env\" \"ext_logging_log_version_1\" (func $ext_logging_log_version_1 (type 12)))\n"
      "  (import \"env\" \"ext_logging_max_level_version_1\" (func $ext_logging_max_level_version_1 (type 35)))\n"

      /// below is start function with import function call defined in test case
      "  (type $v (func))\n"
      "  (start $starter)\n"
      "  (func $starter (; 11 ;) (type 11)\n"
      "%s" // to plug actual call
      "  )\n"
      ")";
  // clang-format on
};

/**
 * For all tests:
 * @given runtime external interface with mocked externals
 * @when external function is invoked with provided arguments from WASM
 * @then corresponding host function is invoked with provided arguments
 */

TEST_F(REITest, ext_blake2_256_enumerated_trie_root_Test) {
  WasmSpan values = PtrSize{12, 42}.combine();
  WasmPointer result = 321;

  EXPECT_CALL(*host_api_, ext_trie_blake2_256_ordered_root_version_1(values))
      .WillOnce(Return(result));
  auto execute_code =
      (boost::format("(call $assert_eq_i32\n"
                     "    (call $ext_trie_blake2_256_ordered_root_version_1\n"
                     "      (i64.const %d)\n"
                     "    )\n"
                     "    (i32.const %d)\n"
                     ")\n")
       % values % result)
          .str();
  executeWasm(execute_code);
}

TEST_F(REITest, ext_storage_changes_root_Test) {
  WasmPointer parent_hash_data = 123;
  WasmSize parent_hash_len = 42;
  WasmPointer result = 321;

  EXPECT_CALL(*host_api_, ext_storage_changes_root_version_1(PtrSize(parent_hash_data, parent_hash_len).combine()))
      .WillOnce(Return(result));

  auto execute_code =
      (boost::format("    (call $assert_eq_i32\n"
                     "      (call $ext_storage_changes_root_version_1\n"
                     "        (i64.const %d)\n"
                     "      )\n"
                     "      (i32.const %d)\n"
                     "    )\n")
       % PtrSize(parent_hash_data, parent_hash_len).combine() % result)
          .str();
  SCOPED_TRACE("ext_storage_changes_root_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_storage_root_Test) {
  WasmPointer storage_root = 12;

  EXPECT_CALL(*host_api_, ext_storage_root_version_1()).WillOnce(Return(storage_root));
  auto execute_code = (boost::format("    (call $assert_eq_i32\n"
                                     "      (call $ext_storage_root_version_1)\n"
                                     "      (i32.const %d)\n"
                                     "    )\n") % storage_root).str();
  executeWasm(execute_code);
}

TEST_F(REITest, ext_logging_log_version_1_Test) {
  PtrSize position(12, 12);
  const auto pos_packed = position.combine();
  WasmEnum ll = static_cast<WasmEnum>(WasmLogLevel::Error);

  EXPECT_CALL(*host_api_, ext_logging_log_version_1(ll, pos_packed, pos_packed))
      .Times(1);
  auto execute_code = (boost::format("    (call $ext_logging_log_version_1\n"
                                     "      (i32.const %d)\n"
                                     "      (i64.const %d)\n"
                                     "      (i64.const %d)\n"
                                     "    )\n")
                       % ll % position.combine() % position.combine())
                          .str();
  executeWasm(execute_code);
}

/**
 * @given wasm runtime ext_logging_max_level_version_1
 * @when try to get max log level
 * @then correct log level returned once
 */
TEST_F(REITest, ext_logging_max_level_version_1_Test) {
  WasmEnum expected_res = static_cast<WasmEnum>(WasmLogLevel::Info);

  EXPECT_CALL(*host_api_, ext_logging_max_level_version_1())
      .WillOnce(Return(expected_res));
  auto execute_code =
      (boost::format("    (call $assert_eq_i32\n"
                     "      (call $ext_logging_max_level_version_1)\n"
                     "      (i32.const %d)\n"
                     "    )\n")
       % expected_res)
          .str();
  SCOPED_TRACE("ext_logging_max_level_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_ed25519_public_keys_v1_Test) {
  WasmSize key_type = KEY_TYPE_BABE;
  WasmSpan res = PtrSize(1, 2).combine();

  EXPECT_CALL(*host_api_, ext_crypto_ed25519_public_keys_version_1(key_type))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i64\n"
                     "      (call $ext_crypto_ed25519_public_keys_version_1\n"
                     "        (i32.const %d)\n"
                     "      )\n"
                     "      (i64.const %d)\n"
                     "    )\n")
       % key_type % res)
          .str();
  SCOPED_TRACE("ext_ed25519_public_keys_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_ed25519_generate_v1_Test) {
  WasmSize key_type = KEY_TYPE_BABE;
  WasmSpan seed = PtrSize(1, 2).combine();

  WasmPointer res = 4;

  EXPECT_CALL(*host_api_, ext_crypto_ed25519_generate_version_1(key_type, seed))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i32\n"
                     "      (call $ext_crypto_ed25519_generate_version_1\n"
                     "        (i32.const %d)\n"
                     "        (i64.const %d)\n"
                     "      )\n"
                     "      (i32.const %d)\n"
                     "    )\n")
       % key_type % seed % res)
          .str();
  SCOPED_TRACE("ext_ed25519_generate_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_ed25519_sign_v1_Test) {
  WasmSize key_type = KEY_TYPE_BABE;
  WasmPointer key = 1;
  WasmSpan msg = PtrSize(33, 2).combine();
  WasmSpan res = PtrSize(35, 25).combine();

  EXPECT_CALL(*host_api_, ext_crypto_ed25519_sign_version_1(key_type, key, msg))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i64\n"
                     "      (call $ext_crypto_ed25519_sign_version_1\n"
                     "        (i32.const %d)\n"
                     "        (i32.const %d)\n"
                     "        (i64.const %d)\n"
                     "      )\n"
                     "      (i64.const %d)\n"
                     "    )\n")
       % key_type % key % msg % res)
          .str();
  SCOPED_TRACE("ext_ed25519_generate_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_ed25519_verify_v1_Test) {
  WasmPointer msg_data = 123;
  WasmSize msg_len = 1233;
  WasmSpan msg = PtrSize(msg_data, msg_len).combine();
  WasmPointer sig_data = 42;
  WasmPointer pubkey_data = 321;

  WasmSize res = 1;

  EXPECT_CALL(*host_api_,
              ext_crypto_ed25519_verify_version_1(sig_data, msg, pubkey_data))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i32\n"
                     "      (call $ext_crypto_ed25519_verify_version_1\n"
                     "        (i32.const %d)\n"
                     "        (i64.const %d)\n"
                     "        (i32.const %d)\n"
                     "      )\n"
                     "      (i32.const %d)\n"
                     "    )\n")
       % sig_data % msg % pubkey_data % res)
          .str();
  SCOPED_TRACE("ext_ed25519_verify_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_sr25519_public_keys_v1_Test) {
  WasmSize key_type = KEY_TYPE_BABE;

  WasmSpan res = PtrSize(1, 2).combine();

  EXPECT_CALL(*host_api_, ext_crypto_sr25519_public_keys_version_1(key_type))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i64\n"
                     "      (call $ext_crypto_sr25519_public_keys_version_1\n"
                     "        (i32.const %d)\n"
                     "      )\n"
                     "      (i64.const %d)\n"
                     "    )\n")
       % key_type % res)
          .str();
  SCOPED_TRACE("ext_sr25519_public_keys_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_sr25519_generate_v1_Test) {
  WasmSize key_type = KEY_TYPE_BABE;
  WasmSpan seed = PtrSize(1, 2).combine();

  WasmPointer res = 4;

  EXPECT_CALL(*host_api_, ext_crypto_sr25519_generate_version_1(key_type, seed))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i32\n"
                     "      (call $ext_crypto_sr25519_generate_version_1\n"
                     "        (i32.const %d)\n"
                     "        (i64.const %d)\n"
                     "      )\n"
                     "      (i32.const %d)\n"
                     "    )\n")
       % key_type % seed % res)
          .str();
  SCOPED_TRACE("ext_sr25519_generate_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_sr25519_sign_v1_Test) {
  WasmSize key_type = KEY_TYPE_BABE;
  WasmPointer key = 1;
  WasmSpan msg = PtrSize(33, 2).combine();
  WasmSpan res = PtrSize(35, 25).combine();

  EXPECT_CALL(*host_api_, ext_crypto_sr25519_sign_version_1(key_type, key, msg))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i64\n"
                     "      (call $ext_crypto_sr25519_sign_version_1\n"
                     "        (i32.const %d)\n"
                     "        (i32.const %d)\n"
                     "        (i64.const %d)\n"
                     "      )\n"
                     "      (i64.const %d)\n"
                     "    )\n")
       % key_type % key % msg % res)
          .str();
  SCOPED_TRACE("ext_sr25519_sign_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_sr25519_verify_v2_Test) {
  WasmPointer msg_data = 123;
  WasmSize msg_len = 1233;
  WasmSpan msg = PtrSize(msg_data, msg_len).combine();
  WasmPointer sig_data = 42;
  WasmPointer pubkey_data = 321;

  WasmSize res = 1;

  EXPECT_CALL(*host_api_,
              ext_crypto_sr25519_verify_version_2(sig_data, msg, pubkey_data))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i32\n"
                     "      (call $ext_crypto_sr25519_verify_version_2\n"
                     "        (i32.const %d)\n"
                     "        (i64.const %d)\n"
                     "        (i32.const %d)\n"
                     "      )\n"
                     "      (i32.const %d)\n"
                     "    )\n")
       % sig_data % msg % pubkey_data % res)
          .str();
  SCOPED_TRACE("ext_sr25519_verify_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_crypto_secp256k1_ecdsa_recover_version_1_Test) {
  WasmPointer sig_ptr = 12;
  WasmPointer msg_ptr = 77;
  WasmSpan out_span = PtrSize{109, 41}.combine();

  EXPECT_CALL(*host_api_,
              ext_crypto_secp256k1_ecdsa_recover_version_1(sig_ptr, msg_ptr))
      .WillOnce(Return(out_span));
  auto execute_code =
      (boost::format("(call $assert_eq_i64\n"
                     "    (call $ext_crypto_secp256k1_ecdsa_recover_version_1\n"
                     "      (i32.const %d)\n"
                     "      (i32.const %d)\n"
                     "    )\n"
                     "    (i64.const %d)\n"
                     ")")
       % sig_ptr % msg_ptr % out_span)
          .str();
  executeWasm(execute_code);
}

TEST_F(REITest, ext_crypto_secp256k1_ecdsa_recover_compressed_version_1_Test) {
  WasmPointer sig_ptr = 12;
  WasmPointer msg_ptr = 77;
  WasmSpan out_span = PtrSize{109, 41}.combine();

  EXPECT_CALL(
      *host_api_,
      ext_crypto_secp256k1_ecdsa_recover_compressed_version_1(sig_ptr, msg_ptr))
      .WillOnce(Return(out_span));
  auto execute_code =
      (boost::format(
           "(call $assert_eq_i64\n"
           "    (call "
           "$ext_crypto_secp256k1_ecdsa_recover_compressed_version_1\n"
           "      (i32.const %d)\n"
           "      (i32.const %d)\n"
           "    )\n"
           "    (i64.const %d)\n"
           ")")
       % sig_ptr % msg_ptr % out_span)
          .str();
  executeWasm(execute_code);
}

TEST_F(REITest, ext_hashing_keccak_256_version_1_Test) {
  WasmPointer res = 3;
  WasmSpan param = PtrSize(1, 2).combine();

  EXPECT_CALL(*host_api_, ext_hashing_keccak_256_version_1(param))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i32\n"
                     "      (call $ext_hashing_keccak_256_version_1\n"
                     "        (i64.const %d)\n"
                     "      )\n"
                     "      (i32.const %d)\n"
                     "    )\n")
       % param % res)
          .str();
  SCOPED_TRACE("ext_hashing_keccak_256_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_hashing_sha2_256_version_1_Test) {
  WasmPointer res = 3;
  WasmSpan param = PtrSize(1, 2).combine();

  EXPECT_CALL(*host_api_, ext_hashing_sha2_256_version_1(param))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i32\n"
                     "      (call $ext_hashing_sha2_256_version_1\n"
                     "        (i64.const %d)\n"
                     "      )\n"
                     "      (i32.const %d)\n"
                     "    )\n")
       % param % res)
          .str();
  SCOPED_TRACE("ext_hashing_sha2_256_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_hashing_blake2_128_version_1_Test) {
  WasmPointer res = 3;
  WasmSpan param = PtrSize(1, 2).combine();

  EXPECT_CALL(*host_api_, ext_hashing_blake2_128_version_1(param))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i32\n"
                     "      (call $ext_hashing_blake2_128_version_1\n"
                     "        (i64.const %d)\n"
                     "      )\n"
                     "      (i32.const %d)\n"
                     "    )\n")
       % param % res)
          .str();
  SCOPED_TRACE("ext_hashing_blake2_128_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_hashing_blake2_256_version_1_Test) {
  WasmPointer res = 3;
  WasmSpan param = PtrSize(1, 2).combine();

  EXPECT_CALL(*host_api_, ext_hashing_blake2_256_version_1(param))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i32\n"
                     "      (call $ext_hashing_blake2_256_version_1\n"
                     "        (i64.const %d)\n"
                     "      )\n"
                     "      (i32.const %d)\n"
                     "    )\n")
       % param % res)
          .str();
  SCOPED_TRACE("ext_hashing_blake2_256_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_hashing_twox_256_version_1_Test) {
  WasmPointer res = 3;
  WasmSpan param = PtrSize(1, 2).combine();

  EXPECT_CALL(*host_api_, ext_hashing_twox_256_version_1(param))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i32\n"
                     "      (call $ext_hashing_twox_256_version_1\n"
                     "        (i64.const %d)\n"
                     "      )\n"
                     "      (i32.const %d)\n"
                     "    )\n")
       % param % res)
          .str();
  SCOPED_TRACE("ext_hashing_twox_256_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_hashing_twox_128_version_1_Test) {
  WasmPointer res = 3;
  WasmSpan param = PtrSize(1, 2).combine();

  EXPECT_CALL(*host_api_, ext_hashing_twox_128_version_1(param))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i32\n"
                     "      (call $ext_hashing_twox_128_version_1\n"
                     "        (i64.const %d)\n"
                     "      )\n"
                     "      (i32.const %d)\n"
                     "    )\n")
       % param % res)
          .str();
  SCOPED_TRACE("ext_hashing_twox_128_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_hashing_twox_64_version_1_Test) {
  WasmPointer res = 3;
  WasmSpan param = PtrSize(1, 2).combine();

  EXPECT_CALL(*host_api_, ext_hashing_twox_64_version_1(param))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i32\n"
                     "      (call $ext_hashing_twox_64_version_1\n"
                     "        (i64.const %d)\n"
                     "      )\n"
                     "      (i32.const %d)\n"
                     "    )\n")
       % param % res)
          .str();
  SCOPED_TRACE("ext_hashing_twox_64_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_allocator_malloc_version_1_Test) {
  WasmSize size = 42;
  WasmPointer ptr = 123;
  EXPECT_CALL(*host_api_, ext_allocator_malloc_version_1(size))
      .WillOnce(Return(ptr));
  auto execute_code =
      (boost::format("    (call $assert_eq_i32\n"
                     "      (call $ext_allocator_malloc_version_1\n"
                     "        (i32.const %d)\n"
                     "      )\n"
                     "      (i32.const %d)\n"
                     "    )\n")
       % size % ptr)
          .str();
  SCOPED_TRACE("ext_allocator_malloc_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_allocator_free_version_1_Test) {
  WasmPointer ptr = 123;
  EXPECT_CALL(*host_api_, ext_allocator_free_version_1(ptr)).Times(1);
  auto execute_code = (boost::format("    (call $ext_allocator_free_version_1\n"
                                     "      (i32.const %d)\n"
                                     "    )\n")
                       % ptr)
                          .str();
  SCOPED_TRACE("ext_allocator_free_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_storage_set_version_1_Test) {
  WasmSpan param1 = PtrSize(1, 2).combine();
  WasmSpan param2 = PtrSize(3, 4).combine();
  EXPECT_CALL(*host_api_, ext_storage_set_version_1(param1, param2))
      .WillOnce(Return());

  auto execute_code = (boost::format("      (call $ext_storage_set_version_1\n"
                                     "        (i64.const %d)\n"
                                     "        (i64.const %d)\n"
                                     "      )\n")
                       % param1 % param2)
                          .str();
  SCOPED_TRACE("ext_storage_set_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_storage_get_version_1_Test) {
  WasmSize key_type = KEY_TYPE_BABE;
  WasmSpan res = PtrSize(1, 2).combine();

  EXPECT_CALL(*host_api_, ext_storage_get_version_1(key_type))
      .WillOnce(Return(res));

  auto execute_code = (boost::format("    (call $assert_eq_i64\n"
                                     "      (call $ext_storage_get_version_1\n"
                                     "        (i64.const %d)\n"
                                     "      )\n"
                                     "      (i64.const %d)\n"
                                     "    )\n")
                       % key_type % res)
                          .str();
  SCOPED_TRACE("ext_storage_get_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_storage_clear_version_1_Test) {
  uint64_t num = 12;

  EXPECT_CALL(*host_api_, ext_storage_clear_version_1(num)).Times(1);
  auto execute_code = (boost::format("    (call $ext_storage_clear_version_1\n"
                                     "      (i64.const %d)\n"
                                     "    )\n")
                       % num)
                          .str();
  SCOPED_TRACE("ext_storage_clear_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_storage_exists_version_1_Test) {
  WasmPointer res = 3;
  WasmSpan param = PtrSize(1, 2).combine();

  EXPECT_CALL(*host_api_, ext_storage_exists_version_1(param))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i32\n"
                     "      (call $ext_storage_exists_version_1\n"
                     "        (i64.const %d)\n"
                     "      )\n"
                     "      (i32.const %d)\n"
                     "    )\n")
       % param % res)
          .str();
  SCOPED_TRACE("ext_storage_exists_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_storage_read_version_1_Test) {
  PtrSize key(123, 1233);
  PtrSize value(42, 12);
  WasmOffset offset(1);
  WasmSpan res = PtrSize(1, 2).combine();

  EXPECT_CALL(
      *host_api_,
      ext_storage_read_version_1(key.combine(), value.combine(), offset))
      .WillOnce(Return(res));
  auto execute_code = (boost::format("    (call $assert_eq_i64\n"
                                     "    (call $ext_storage_read_version_1\n"
                                     "      (i64.const %d)\n"
                                     "      (i64.const %d)\n"
                                     "      (i32.const %d)\n"
                                     "    )\n"
                                     "      (i64.const %d)\n"
                                     "    )\n")
                       % key.combine() % value.combine() % offset % res)
                          .str();
  SCOPED_TRACE("ext_storage_read_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_storage_clear_prefix_version_1_Test) {
  uint64_t num = 12;

  EXPECT_CALL(*host_api_, ext_storage_clear_prefix_version_1(num)).Times(1);
  auto execute_code =
      (boost::format("    (call $ext_storage_clear_prefix_version_1\n"
                     "      (i64.const %d)\n"
                     "    )\n")
       % num)
          .str();
  SCOPED_TRACE("ext_storage_clear_prefix_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_storage_changes_root_version_1_Test) {
  WasmSize key_type = KEY_TYPE_BABE;
  WasmSpan res = 2;

  EXPECT_CALL(*host_api_, ext_storage_changes_root_version_1(key_type))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i64\n"
                     "      (call $ext_storage_changes_root_version_1\n"
                     "        (i64.const %d)\n"
                     "      )\n"
                     "      (i64.const %d)\n"
                     "    )\n")
       % key_type % res)
          .str();
  SCOPED_TRACE("ext_storage_changes_root_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_storage_root_version_1_Test) {
  WasmSpan res = 123141;

  EXPECT_CALL(*host_api_, ext_storage_root_version_1()).WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i64\n"
                     "      (call $ext_storage_root_version_1)\n"
                     "      (i64.const %d)\n"
                     "    )\n")
       % res)
          .str();
  SCOPED_TRACE("ext_storage_root_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_storage_next_key_version_1_Test) {
  WasmSpan param = 5678;
  WasmSpan res = 123141;

  EXPECT_CALL(*host_api_, ext_storage_next_key_version_1(param))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i64\n"
                     "      (call $ext_storage_next_key_version_1"
                     "        (i64.const %d)\n"
                     "      )\n"
                     "      (i64.const %d)\n"
                     "    )\n")
       % param % res)
          .str();
  SCOPED_TRACE("ext_storage_next_key_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_trie_blake2_256_root_version_1_Test) {
  WasmPointer res = 3;
  WasmSpan param = PtrSize(1, 2).combine();

  EXPECT_CALL(*host_api_, ext_trie_blake2_256_root_version_1(param))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i32\n"
                     "      (call $ext_trie_blake2_256_root_version_1\n"
                     "        (i64.const %d)\n"
                     "      )\n"
                     "      (i32.const %d)\n"
                     "    )\n")
       % param % res)
          .str();
  SCOPED_TRACE("ext_trie_blake2_256_root_version_1_Test");
  executeWasm(execute_code);
}

TEST_F(REITest, ext_trie_blake2_256_ordered_root_version_1_Test) {
  WasmPointer res = 3;
  WasmSpan param = PtrSize(1, 2).combine();

  EXPECT_CALL(*host_api_, ext_trie_blake2_256_ordered_root_version_1(param))
      .WillOnce(Return(res));

  auto execute_code =
      (boost::format("    (call $assert_eq_i32\n"
                     "      (call $ext_trie_blake2_256_ordered_root_version_1\n"
                     "        (i64.const %d)\n"
                     "      )\n"
                     "      (i32.const %d)\n"
                     "    )\n")
       % param % res)
          .str();
  SCOPED_TRACE("ext_trie_blake2_256_ordered_root_version_1_Test");
  executeWasm(execute_code);
}
