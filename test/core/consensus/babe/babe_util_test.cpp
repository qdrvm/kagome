/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <chrono>

#include "consensus/babe/impl/babe_util_impl.hpp"
#include "mock/core/clock/clock_mock.hpp"
#include "mock/core/consensus/babe/babe_config_repository_mock.hpp"
#include "primitives/babe_configuration.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;
using namespace clock;
using namespace consensus;
using namespace babe;

using std::chrono_literals::operator""ms;
using testing::Return;
using testing::ReturnRef;

class BabeUtilTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    babe_config_.slot_duration = 6000ms;
    babe_config_.epoch_length = 2;

    babe_config_repo_ =
        std::make_shared<consensus::babe::BabeConfigRepositoryMock>();
    ON_CALL(*babe_config_repo_, config())
        .WillByDefault(ReturnRef(babe_config_));

    clock_ = std::make_shared<SystemClockMock>();
    babe_util_ = std::make_shared<BabeUtilImpl>(babe_config_repo_, *clock_);
  }

  primitives::BabeConfiguration babe_config_;
  std::shared_ptr<BabeConfigRepositoryMock> babe_config_repo_;
  std::shared_ptr<SystemClockMock> clock_;
  std::shared_ptr<BabeUtil> babe_util_;
};

/**
 * @given current time
 * @when getCurrentSlot is called
 * @then compare slot estimations
 */
TEST_F(BabeUtilTest, getCurrentSlot) {
  auto time = std::chrono::system_clock::now();
  EXPECT_CALL(*clock_, now()).Times(1).WillOnce(Return(time));
  EXPECT_EQ(static_cast<BabeSlotNumber>(time.time_since_epoch()
                                        / babe_config_.slot_duration),
            babe_util_->getCurrentSlot());
}
