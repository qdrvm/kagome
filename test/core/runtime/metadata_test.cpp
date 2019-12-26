/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/grandpa_impl.hpp"

#include <gtest/gtest.h>
#include "core/runtime/runtime_test.hpp"
#include "extensions/extension_impl.hpp"
#include "runtime/binaryen/runtime_api/metadata_impl.hpp"
#include "runtime/common/wasm_memory_impl.hpp"
#include "testutil/outcome.hpp"

using ::testing::_;
using ::testing::Return;

using kagome::runtime::Metadata;
using kagome::runtime::binaryen::MetadataImpl;

namespace fs = boost::filesystem;

class MetadataTest : public RuntimeTest {
 public:
  void SetUp() override {
    RuntimeTest::SetUp();

    api_ = std::make_shared<MetadataImpl>(wasm_provider_, extension_);
  }

 protected:
  std::shared_ptr<Metadata> api_;
};

/**
 * @given initialized Metadata api
 * @when metadata() is invoked
 * @then successful result is returned
 */
TEST_F(MetadataTest, metadata) {
  ASSERT_TRUE(api_->metadata());
}
