/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/parachain_host_impl.hpp"

#include <gtest/gtest.h>
#include "extensions/extension_impl.hpp"
#include "primitives/impl/scale_codec_impl.hpp"
#include "runtime/impl/wasm_memory_impl.hpp"
#include "testutil/outcome.hpp"
#include "testutil/runtime/wasm_test.hpp"
#include "core/runtime/runtime_test.hpp"

using kagome::common::Buffer;
using kagome::extensions::ExtensionImpl;
using kagome::primitives::ScaleCodecImpl;
using kagome::runtime::ParachainHost;
using kagome::runtime::ParachainHostImpl;
using kagome::primitives::parachain::ParaId;
using kagome::primitives::parachain::DutyRoster;
using kagome::primitives::parachain::ValidatorId;
using kagome::primitives::parachain::Relay;
using kagome::primitives::parachain::Chain;
using kagome::primitives::parachain::Parachain;

using ::testing::_;
using ::testing::Return;

namespace fs = boost::filesystem;

class ParachainHostTest: public RuntimeTest {
 public:
  void SetUp() override {
    RuntimeTest::SetUp();

    api_ = std::make_shared<ParachainHostImpl>(state_code_, extension_, codec_);
  }

  ParaId createParachainId() const {
    return 1ul;
  }

 protected:
  std::shared_ptr<ParachainHost> api_;
};

/**
 * @given initialized parachain host api
 * @when dutyRoster() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_DutyRosterTest) {
  ASSERT_TRUE(api_->dutyRoster());
}

/**
 * @given initialized parachain host api
 * @when activeParachains() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_ActiveParachainsTest) {
  ASSERT_TRUE(api_->activeParachains());
}

/**
 * @given initialized parachain host api
 * @when parachainHead() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_ParachainHeadTest) {
  auto id = createParachainId();
  ASSERT_TRUE(api_->parachainHead(id));
}

/**
 * @given initialized parachain host api
 * @when parachainCode() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_ParachainCodeTest) {
  auto id = createParachainId();
  ASSERT_TRUE(api_->parachainCode(id));
}

/**
 * @given initialized parachain host api
 * @when validators() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_ValidatorsTest) {
  auto id = createBlockId();
  ASSERT_TRUE(api_->validators(id));
}
