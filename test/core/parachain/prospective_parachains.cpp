/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

#include "crypto/hasher/hasher_impl.hpp"
#include "parachain/types.hpp"
#include "parachain/validator/prospective_parachains.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"
#include "scale/kagome_scale.hpp"
#include "scale/scale.hpp"
#include "testutil/scale_test_comparator.hpp"

using namespace kagome::primitives;
using namespace kagome::parachain;
namespace network = kagome::network;
namespace runtime = kagome::runtime;
namespace common = kagome::common;

struct PerParaData {
  BlockNumber min_relay_parent;
  HeadData head_data;
  std::vector<fragment::CandidatePendingAvailability> pending_availability;
};

struct TestState {
  std::vector<runtime::CoreState> availability_cores;
  ValidationCodeHash validation_code_hash;

  TestState()
      : availability_cores{{runtime::ScheduledCore{.para_id = ParachainId{1},
                                                   .collator = std::nullopt},
                            runtime::ScheduledCore{.para_id = ParachainId{2},
                                                   .collator = std::nullopt}}},
        validation_code_hash{Hash::fromString("42").value()} {}
};

class ProspectiveParachainsTest : public testing::Test {
  void SetUp() override {
    hasher_ = std::make_shared<kagome::crypto::HasherImpl>();
    /// OUTCOME_TRY(enc_header, testutil::scaleEncodeAndCompareWithRef(header));
    /// auto hash = hasher_->blake2b_256(enc_header);
  }

 protected:
  std::shared_ptr<kagome::crypto::Hasher> hasher_;
};

TEST_F(ProspectiveParachainsTest, sendCandidatesAndCheckIfFound) {
  network::ExView{
      .view = {},
      .new_head =
          BlockHeader{
              .number = 100,
              .parent_hash = hasher_->blake2b_256(
                  testutil::scaleEncodeAndCompareWithRef(BlockNumber{99})
                      .value()),
              .state_root = {},
              .extrinsics_root = {},
              .digest = {},
              .hash_opt = {},
          },
      .lost = {},
  };
}
