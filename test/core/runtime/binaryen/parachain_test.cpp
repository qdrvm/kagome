/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
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
using kagome::primitives::events::ChainSubscriptionEngine;
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

namespace fs = kagome::filesystem;

class ParachainHostTest : public BinaryenRuntimeTest {
 public:
  void SetUp() override {
    BinaryenRuntimeTest::SetUp();

    api_ = std::make_shared<ParachainHostImpl>(
        executor_, std::make_shared<ChainSubscriptionEngine>());
  }

  ParaId createParachainId() const {
    return 1ul;
  }

 protected:
  std::shared_ptr<ParachainHost> api_;
};

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

/**
 * @given initialized parachain host api
 * @when validator_groups() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_ValidatorGroupsTest) {
  ASSERT_TRUE(api_->validator_groups("block_hash"_hash256));
}

/**
 * @given initialized parachain host api
 * @when availability_cores() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_AvailabilityCoresTest) {
  ASSERT_TRUE(api_->availability_cores("block_hash"_hash256));
}

/**
 * @given initialized parachain host api
 * @when persisted_validation_data() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_PersistedValidationDataTest) {
  auto id = createParachainId();
  kagome::runtime::OccupiedCoreAssumption assumption{};
  ASSERT_TRUE(
      api_->persisted_validation_data("block_hash"_hash256, id, assumption));
}

/**
 * @given initialized parachain host api
 * @when check_validation_outputs() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_CheckValidationOutputsTest) {
  auto id = createParachainId();
  kagome::runtime::CandidateCommitments outputs;
  ASSERT_TRUE(
      api_->check_validation_outputs("block_hash"_hash256, id, outputs));
}

/**
 * @given initialized parachain host api
 * @when session_index_for_child() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_SessionIndexForChildTest) {
  ASSERT_TRUE(api_->session_index_for_child("block_hash"_hash256));
}

/**
 * @given initialized parachain host api
 * @when validation_code() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_ValidationCodeTest) {
  auto id = createParachainId();
  kagome::runtime::OccupiedCoreAssumption assumption{};
  ASSERT_TRUE(api_->validation_code("block_hash"_hash256, id, assumption));
}

/**
 * @given initialized parachain host api
 * @when validation_code_by_hash() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_ValidationCodeByHashTest) {
  kagome::runtime::ValidationCodeHash hash = "runtime_hash"_hash256;
  ASSERT_TRUE(api_->validation_code_by_hash("block_hash"_hash256, hash));
}

/**
 * @given initialized parachain host api
 * @when candidate_pending_availability() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_CandidatePendingAvailabilityTest) {
  auto id = createParachainId();
  ASSERT_TRUE(api_->candidate_pending_availability("block_hash"_hash256, id));
}

/**
 * @given initialized parachain host api
 * @when candidate_events() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_CandidateEventsTest) {
  ASSERT_TRUE(api_->candidate_events("block_hash"_hash256));
}

/**
 * @given initialized parachain host api
 * @when session_info() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_SessionInfoTest) {
  kagome::runtime::SessionIndex index{};
  ASSERT_TRUE(api_->session_info("block_hash"_hash256, index));
}

/**
 * @given initialized parachain host api
 * @when dmq_contents() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_DmqContentsTest) {
  auto id = createParachainId();
  ASSERT_TRUE(api_->dmq_contents("block_hash"_hash256, id));
}

/**
 * @given initialized parachain host api
 * @when inbound_hrmp_channels_contents() is invoked
 * @then successful result is returned
 */
TEST_F(ParachainHostTest, DISABLED_InboundHrmpChannelsContentsTest) {
  auto id = createParachainId();
  ASSERT_TRUE(api_->inbound_hrmp_channels_contents("block_hash"_hash256, id));
}
