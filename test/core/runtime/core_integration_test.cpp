/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/core_impl.hpp"

#include <fstream>

#include <gtest/gtest.h>
#include "core/storage/merkle/mock_trie_db.hpp"
#include "extensions/extension_impl.hpp"
#include "primitives/impl/scale_codec_impl.hpp"
#include "runtime/impl/wasm_memory_impl.hpp"

using kagome::common::Buffer;
using kagome::extensions::ExtensionImpl;
using kagome::primitives::ScaleCodecImpl;
using kagome::runtime::CoreImpl;
using kagome::runtime::WasmMemoryImpl;
using kagome::storage::merkle::MockTrieDb;
using kagome::primitives::Block;

Buffer getRuntimeCode() {
  std::string path =
      "../../test/core/runtime/wasm/polkadot_runtime.compact.wasm";
  std::ifstream ifd(path, std::ios::binary | std::ios::ate);
  int size = ifd.tellg();
  ifd.seekg(0, std::ios::beg);
  std::vector<char> buffer;
  buffer.resize(size);  // << resize not reserve
  ifd.read(buffer.data(), size);
  gsl::span<uint8_t> span((uint8_t *)buffer.data(), buffer.size());
  return Buffer(span);
}

TEST(CoreTest, VersionTest) {
  auto state_code = getRuntimeCode();

  auto trie_db = std::make_shared<MockTrieDb>();
  auto memory = std::make_shared<WasmMemoryImpl>();

  auto extension = std::make_shared<ExtensionImpl>(memory, trie_db);
  auto codec = std::make_shared<ScaleCodecImpl>();

  CoreImpl core_api(state_code, extension, codec);

  auto version = core_api.version();
  std::cout << std::endl
            << "name: " << version.implName() << std::endl
            << "version: " << version.implVersion() << std::endl
            << "specName: " << version.specName() << std::endl;
}

TEST(CoreTest, ExecuteBlockTest) {
  Block
}

