/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/parachain_host.hpp"

#include <gtest/gtest.h>

#include "core/runtime/binaryen/binaryen_runtime_test.hpp"
#include "host_api/impl/host_api_impl.hpp"
#include "runtime/binaryen/memory_impl.hpp"
#include "testutil/outcome.hpp"

using kagome::common::Buffer;
using kagome::host_api::HostApiImpl;
using kagome::primitives::parachain::Chain;
using kagome::primitives::parachain::DutyRoster;
using kagome::primitives::parachain::Parachain;
using kagome::primitives::parachain::ParaId;
using kagome::primitives::parachain::Relay;
using kagome::primitives::parachain::ValidatorId;
using kagome::runtime::ParachainHost;
using kagome::runtime::ParachainHostImpl;

using ::testing::_;
using ::testing::Return;

namespace fs = boost::filesystem;

class ParachainHostTest : public BinaryenRuntimeTest {
 public:
  void SetUp() override {
    BinaryenRuntimeTest::SetUp();

    api_ = std::make_shared<ParachainHostImpl>(executor_);
  }

  ParaId createParachainId() const {
    return 1ul;
  }

 protected:
  std::shared_ptr<ParachainHost> api_;
};

// TODO(yuraz): PRE-157 find out do we need to give block_id to api functions
/**
 * @given initialized parachain host api
 * @when dutyRoster() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_DutyRosterTest) {
  ASSERT_TRUE(api_->duty_roster("block_hash"_hash256));
}

/**
 * @given initialized parachain host api
 * @when activeParachains() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_ActiveParachainsTest) {
  ASSERT_TRUE(api_->active_parachains("block_hash"_hash256));
}

/**
 * @given initialized parachain host api
 * @when parachainHead() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_ParachainHeadTest) {
  auto id = createParachainId();
  ASSERT_TRUE(api_->parachain_head("block_hash"_hash256, id));
}

/**
 * @given initialized parachain host api
 * @when parachain_code() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_ParachainCodeTest) {
  auto id = createParachainId();
  ASSERT_TRUE(api_->parachain_code("block_hash"_hash256, id));
}

/**
 * @given initialized parachain host api
 * @when validators() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_ValidatorsTest) {
  ASSERT_TRUE(api_->validators("block_hash"_hash256));
}
/*
TEST_F(ParachainHostTest, DISABLED_ValidatorGroupsTest) {
  ASSERT_TRUE(api_->validator_groups("block_hash"_hash256));
}

TEST_F(ParachainHostTest, DISABLED_AvailabilityCoresTest) {
  ASSERT_TRUE(api_->availability_cores("block_hash"_hash256));
}

TEST_F(ParachainHostTest, DISABLED_PersistedValidationDataTest) {
  ASSERT_TRUE(api_->persisted_validation_data("block_hash"_hash256));
}

TEST_F(ParachainHostTest, DISABLED_CheckValidationOutputsTest) {
  ASSERT_TRUE(api_->check_validation_outputs("block_hash"_hash256));
}

TEST_F(ParachainHostTest, DISABLED_SessionIndexForChildTest) {
  ASSERT_TRUE(api_->session_index_for_child("block_hash"_hash256));
}

TEST_F(ParachainHostTest, DISABLED_ValidationCodeTest) {
  ASSERT_TRUE(api_->validation_code("block_hash"_hash256));
}

TEST_F(ParachainHostTest, DISABLED_ValidationCodeByHashTest) {
  ASSERT_TRUE(api_->validation_code_by_hash("block_hash"_hash256));
}

TEST_F(ParachainHostTest, DISABLED_CandidatePendingAvailabilityTest) {
  ASSERT_TRUE(api_->candidate_pending_availability("block_hash"_hash256));
}

TEST_F(ParachainHostTest, DISABLED_CandidateEventsTest) {
  ASSERT_TRUE(api_->candidate_events("block_hash"_hash256));
}

TEST_F(ParachainHostTest, DISABLED_SessionInfoTest) {
  ASSERT_TRUE(api_->session_info("block_hash"_hash256));
}

TEST_F(ParachainHostTest, DISABLED_DmqContentsTest) {
  ASSERT_TRUE(api_->dmq_contents("block_hash"_hash256));
}*/

/* TEST_F(ParachainHostTest, DISABLED_InboundHrmpChannelsContentsTest) {
  auto id = createParachainId();
  ASSERT_TRUE(api_->inbound_hrmp_channels_contents("block_hash"_hash256, id));
}*/
