/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <mock/libp2p/basic/scheduler_mock.hpp>

#include "consensus/babe/impl/babe_synchronizer_impl.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/consensus/babe/block_executor_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/network/router_mock.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;
using namespace clock;
using namespace consensus;
using namespace storage;

using std::chrono_literals::operator""ms;
using testing::_;
using testing::Return;

class BabeSynchronizerTest : public testing::Test, protected BabeSynchronizerImpl {
 public:
  BabeSynchronizerTest(): BabeSynchronizerImpl({},{},{},{},{},{}) {}

  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    EXPECT_CALL(*app_state_manager, atShutdown(_)).WillOnce(Return());

    synchronizer = std::make_shared<BabeSynchronizerImpl>(app_state_manager,
                                                          block_tree,
                                                          block_executor,
                                                          router,
                                                          scheduler,
                                                          hasher);
  }

  std::shared_ptr<application::AppStateManagerMock> app_state_manager =
      std::make_shared<application::AppStateManagerMock>();
  std::shared_ptr<blockchain::BlockTreeMock> block_tree =
      std::make_shared<blockchain::BlockTreeMock>();
  std::shared_ptr<BlockExecutorMock> block_executor =
      std::make_shared<BlockExecutorMock>();
  std::shared_ptr<network::RouterMock> router =
      std::make_shared<network::RouterMock>();
  std::shared_ptr<libp2p::basic::SchedulerMock> scheduler =
      std::make_shared<libp2p::basic::SchedulerMock>();
  std::shared_ptr<crypto::HasherMock> hasher =
      std::make_shared<crypto::HasherMock>();

  std::shared_ptr<BabeSynchronizerImpl> synchronizer;
};

TEST_F(BabeSynchronizerTest, test) {
  synchronizer->findCommonBlock();
}
