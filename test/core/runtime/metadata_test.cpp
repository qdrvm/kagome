/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "runtime/binaryen/runtime_api/grandpa_impl.hpp"

#include <gtest/gtest.h>

#include "core/runtime/runtime_test.hpp"
#include "extensions/impl/extension_impl.hpp"
#include "runtime/binaryen/runtime_api/metadata_impl.hpp"
#include "runtime/binaryen/wasm_memory_impl.hpp"
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

    api_ = std::make_shared<MetadataImpl>(wasm_provider_, extension_factory_);
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
