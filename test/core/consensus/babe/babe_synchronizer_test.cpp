/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include <mock/libp2p/basic/scheduler_mock.hpp>
#include <stdexcept>

#include "consensus/babe/impl/babe_synchronizer_impl.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/consensus/babe/block_executor_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/network/protocols/sync_protocol_mock.hpp"
#include "mock/core/network/router_mock.hpp"
#include "primitives/common.hpp"
#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;
using namespace clock;
using namespace consensus;
using namespace storage;

using std::chrono_literals::operator""ms;
using primitives::BlockHash;
using primitives::BlockInfo;
using primitives::BlockNumber;
using testing::_;
using testing::Return;

std::tuple<std::vector<BlockInfo>, std::vector<BlockInfo>> generateChains(
    primitives::BlockNumber finalized,
    primitives::BlockNumber common,
    primitives::BlockNumber local_best,
    primitives::BlockNumber remote_best) {
  if (local_best < finalized) {
    throw std::invalid_argument(
        "Local best block must not be before finalized");
  }
  if (local_best < common or remote_best < common) {
    throw std::invalid_argument("Common block must not be after best");
  }

  std::tuple<std::vector<BlockInfo>, std::vector<BlockInfo>> result;

  auto &local_blocks = std::get<0>(result);
  local_blocks.reserve(local_best);
  for (BlockNumber i = 0; i <= local_best; ++i) {
    std::string s = std::to_string(0) + (i <= finalized ? ":F" : ":N")
                    + (i <= common ? ":C" : ":L");
    BlockHash hash{};
    std::copy_n(s.data(), std::min(s.size(), 32ul), hash.begin());
    local_blocks.emplace_back(i, hash);
  }

  auto &remote_blocks = std::get<1>(result);
  remote_blocks.reserve(remote_best);
  for (BlockNumber i = 0; i <= remote_best; ++i) {
    std::string s = std::to_string(0) + (i <= finalized ? ":F" : ":N")
                    + (i <= common ? ":C" : ":R");
    BlockHash hash{};
    std::copy_n(s.data(), std::min(s.size(), 32ul), hash.begin());
    remote_blocks.emplace_back(i, hash);
  }

  return result;
}

class SyncResultHandlerMock {
 public:
  MOCK_METHOD1(call, void(outcome::result<primitives::BlockInfo>));

  void operator()(outcome::result<primitives::BlockInfo> res) {
    call(res);
  }
};

class BabeSynchronizerTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    EXPECT_CALL(*app_state_manager, atShutdown(_)).WillOnce(Return());

    EXPECT_CALL(*router, getSyncProtocol())
        .WillRepeatedly(Return(sync_protocol));

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
  std::shared_ptr<network::SyncProtocolMock> sync_protocol =
      std::make_shared<network::SyncProtocolMock>();
  std::shared_ptr<network::RouterMock> router =
      std::make_shared<network::RouterMock>();
  std::shared_ptr<libp2p::basic::SchedulerMock> scheduler =
      std::make_shared<libp2p::basic::SchedulerMock>();
  std::shared_ptr<crypto::HasherMock> hasher =
      std::make_shared<crypto::HasherMock>();

  std::shared_ptr<BabeSynchronizerImpl> synchronizer;

  libp2p::peer::PeerId peer_id = ""_peerid;
};

TEST_F(BabeSynchronizerTest, DISABLED_findCommonBlock) {
  BlockNumber finalized = 0;
  BlockNumber common = 5;
  BlockNumber local_best = 10;
  BlockNumber remote_best = 15;

  auto [local, remote] =
      generateChains(finalized, common, local_best, remote_best);

  SyncResultHandlerMock mock;
  auto cb = [&](auto res)  { mock(res); };

  auto lower = finalized;
  auto hint = std::min(local_best, remote_best);
  auto upper = std::min(local_best, remote_best) + 1;
  synchronizer->findCommonBlock(peer_id, lower, upper, hint, std::move(cb));
}
