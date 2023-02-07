/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include <mock/libp2p/basic/scheduler_mock.hpp>
#include <stdexcept>

#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/consensus/babe/block_appender_mock.hpp"
#include "mock/core/consensus/babe/block_executor_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/network/protocols/sync_protocol_mock.hpp"
#include "mock/core/network/router_mock.hpp"
#include "mock/core/storage/changes_trie/changes_tracker_mock.hpp"
#include "mock/core/storage/persistent_map_mock.hpp"
#include "mock/core/storage/spaced_storage_mock.hpp"
#include "mock/core/storage/trie/serialization/trie_serializer_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "network/impl/synchronizer_impl.hpp"
#include "primitives/common.hpp"
#include "storage/changes_trie/changes_tracker.hpp"
#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;
using namespace clock;
using namespace consensus::babe;
using namespace storage;

using std::chrono_literals::operator""ms;
using network::Synchronizer;
using primitives::BlockData;
using primitives::BlockHash;
using primitives::BlockHeader;
using primitives::BlockInfo;
using primitives::BlockNumber;
using storage::BufferStorageMock;
using storage::SpacedStorageMock;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Return;
using ::testing::Truly;
using ::testing::Values;

class SyncResultHandlerMock {
 public:
  MOCK_METHOD(void, call, (outcome::result<primitives::BlockInfo>), ());

  void operator()(outcome::result<primitives::BlockInfo> res) {
    call(res);
  }
};

class SynchronizerTest
    : public ::testing::TestWithParam<
          std::tuple<BlockNumber, BlockNumber, BlockNumber, BlockNumber>> {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    EXPECT_CALL(*app_state_manager, atShutdown(_)).WillOnce(Return());

    EXPECT_CALL(*router, getSyncProtocol())
        .WillRepeatedly(Return(sync_protocol));

    EXPECT_CALL(*scheduler, scheduleImplMockCall(_, _, _)).Times(AnyNumber());

    EXPECT_CALL(app_config, syncMethod())
        .WillOnce(Return(application::AppConfiguration::SyncMethod::Full));

    EXPECT_CALL(*spaced_storage, getSpace(kagome::storage::Space::kDefault))
        .WillRepeatedly(Return(buffer_storage));

    synchronizer =
        std::make_shared<network::SynchronizerImpl>(app_config,
                                                    app_state_manager,
                                                    block_tree,
                                                    changes_tracker,
                                                    block_appender,
                                                    block_executor,
                                                    serializer,
                                                    storage,
                                                    router,
                                                    scheduler,
                                                    hasher,
                                                    nullptr,
                                                    nullptr,
                                                    nullptr,
                                                    spaced_storage);
  }

  application::AppConfigurationMock app_config;
  std::shared_ptr<application::AppStateManagerMock> app_state_manager =
      std::make_shared<application::AppStateManagerMock>();
  std::shared_ptr<blockchain::BlockTreeMock> block_tree =
      std::make_shared<blockchain::BlockTreeMock>();
  std::shared_ptr<changes_trie::ChangesTracker> changes_tracker =
      std::make_shared<kagome::storage::changes_trie::ChangesTrackerMock>();
  std::shared_ptr<BlockHeaderAppenderMock> block_appender =
      std::make_shared<BlockHeaderAppenderMock>();
  std::shared_ptr<BlockExecutorMock> block_executor =
      std::make_shared<BlockExecutorMock>();
  std::shared_ptr<trie::TrieStorageMock> storage =
      std::make_shared<trie::TrieStorageMock>();
  std::shared_ptr<trie::TrieSerializerMock> serializer =
      std::make_shared<trie::TrieSerializerMock>();
  std::shared_ptr<network::SyncProtocolMock> sync_protocol =
      std::make_shared<network::SyncProtocolMock>();
  std::shared_ptr<network::RouterMock> router =
      std::make_shared<network::RouterMock>();
  std::shared_ptr<libp2p::basic::SchedulerMock> scheduler =
      std::make_shared<libp2p::basic::SchedulerMock>();
  std::shared_ptr<crypto::HasherMock> hasher =
      std::make_shared<crypto::HasherMock>();
  std::shared_ptr<SpacedStorageMock> spaced_storage =
      std::make_shared<SpacedStorageMock>();
  std::shared_ptr<BufferStorageMock> buffer_storage =
      std::make_shared<BufferStorageMock>();

  std::shared_ptr<network::SynchronizerImpl> synchronizer;

  libp2p::peer::PeerId peer_id = ""_peerid;

  std::tuple<std::vector<BlockInfo>, std::vector<BlockInfo>> generateChains(
      primitives::BlockNumber finalized,
      primitives::BlockNumber common,
      primitives::BlockNumber local_best,
      primitives::BlockNumber remote_best);
};

// Imitates call getBlockHeader based on generated local blockchain
ACTION_P(blockTree_getBlockHeader, local_blocks) {
  auto &block_id = arg0;

  auto block_info = visit_in_place(
      block_id,
      [&](const BlockNumber &n) -> std::optional<BlockInfo> {
        std::cout << "GetHeader: #" << n << ", ";
        if (local_blocks.size() > n) {
          auto &bi = local_blocks[n];
          std::cout << "Result: " << bi.hash.data() << std::endl;
          return bi;
        }
        std::cout << "Result: not found" << std::endl;
        return std::nullopt;
      },
      [&](const BlockHash &h) -> std::optional<BlockInfo> {
        std::cout << "GetHeader: " << h.data() << ", ";
        for (BlockNumber n = 0; n < local_blocks.size(); ++n) {
          if (local_blocks[n].hash == h) {
            auto &bi = local_blocks[n];
            std::cout << "Result: " << bi.hash.data() << std::endl;
            return bi;
          }
        }
        std::cout << "Result: not found" << std::endl;
        return std::nullopt;
      });

  if (not block_info.has_value()) {
    return boost::system::error_code{};
  }

  return BlockHeader{.number = block_info->number};
}

// Imitates response for block request based on generated local blockchain
ACTION_P(syncProtocol_request, remote_blocks) {
  auto &request = arg1;
  auto &handler = arg2;

  auto bi = visit_in_place(
      request.from,
      [&](const BlockNumber &n) -> std::optional<BlockInfo> {
        std::cout << "Requested: #" << n << ", ";
        if (remote_blocks.size() > n) {
          auto &bi = remote_blocks[n];
          std::cout << "Result: " << bi.hash.data() << std::endl;
          return bi;
        }
        std::cout << "Result: not found" << std::endl;
        return std::nullopt;
      },
      [&](const BlockHash &h) -> std::optional<BlockInfo> {
        std::cout << "Requested: " << h.data() << ", ";
        for (BlockNumber n = 0; n < remote_blocks.size(); ++n) {
          if (remote_blocks[n].hash == h) {
            auto &bi = remote_blocks[n];
            std::cout << "Result: " << bi.hash.data() << std::endl;
            return bi;
          }
        }
        std::cout << "Result: not found" << std::endl;
        return std::nullopt;
      });

  network::BlocksResponse response;

  if (bi.has_value()) {
    response.blocks.emplace_back(BlockData{
        .hash = bi->hash, .header = BlockHeader{.number = bi->number}});
  }

  handler(response);
}

/**
 * Generates imitation of local and remote blockchain
 * @param finalized - number of common finalized block
 * @param common - number of best common block for both sides
 * @param local_best - number of local best block
 * @param remote_best - number of local best block
 * @return
 */
std::tuple<std::vector<BlockInfo>, std::vector<BlockInfo>>
SynchronizerTest::generateChains(BlockNumber finalized,
                                 BlockNumber common,
                                 BlockNumber local_best,
                                 BlockNumber remote_best) {
  if (local_best < finalized) {
    throw std::invalid_argument(
        "Local best block must not be before finalized");
  }
  if (local_best < common or remote_best < common) {
    throw std::invalid_argument("Common block must not be after best");
  }

  std::tuple<std::vector<BlockInfo>, std::vector<BlockInfo>> result;

  std::string s = std::to_string(finalized) + ":F:C";
  BlockHash hash{};
  std::copy_n(s.data(), std::min(s.size(), 32ul), hash.begin());

  EXPECT_CALL(*block_tree, getLastFinalized())
      .WillRepeatedly(testing::Return(primitives::BlockInfo(finalized, hash)));

  auto &local_blocks = std::get<0>(result);
  local_blocks.reserve(local_best);
  std::cout << "Local blocks:  ";
  for (BlockNumber i = 0; i <= local_best; ++i) {
    std::string s = std::to_string(i) + (i <= finalized ? 'F' : 'N')
                    + (i <= common ? 'C' : 'L');
    std::cout << s << "  ";

    BlockHash hash{};
    std::copy_n(s.data(), std::min(s.size(), 32ul), hash.begin());
    const auto &b = local_blocks.emplace_back(i, hash);

    if (i == finalized) {
      EXPECT_CALL(*block_tree, getLastFinalized())
          .WillRepeatedly(testing::Return(b));
    }
    if (i == local_best) {
      EXPECT_CALL(*block_tree, getBestContaining(_, _))
          .WillRepeatedly(testing::Return(b));
    }
  }
  std::cout << std::endl;

  EXPECT_CALL(*block_tree, getBlockHeader(_))
      .WillRepeatedly(blockTree_getBlockHeader(local_blocks));

  auto &remote_blocks = std::get<1>(result);
  remote_blocks.reserve(remote_best);
  std::cout << "Remote blocks: ";
  for (BlockNumber i = 0; i <= remote_best; ++i) {
    std::string s = std::to_string(i) + (i <= finalized ? 'F' : 'N')
                    + (i <= common ? 'C' : 'R');
    std::cout << s << "  ";
    BlockHash hash{};
    std::copy_n(s.data(), std::min(s.size(), 32ul), hash.begin());
    [[maybe_unused]] const auto &b = remote_blocks.emplace_back(i, hash);
  }
  std::cout << std::endl;

  EXPECT_CALL(*sync_protocol, request(_, _, _))
      .WillRepeatedly(syncProtocol_request(remote_blocks));

  return result;
}

TEST_P(SynchronizerTest, findCommonBlock) {
  /// @given variants existing blockchain - local and remote
  auto &&[finalized, common, local_best, remote_best] = GetParam();

  auto &&[local, remote] =
      generateChains(finalized, common, local_best, remote_best);

  // Mocked callback
  SyncResultHandlerMock mock;

  /// @then callback will be called once with expected data
  auto is_expected = [&local = local, &common = common](const auto &res) {
    return res.has_value() and res.value() == local[common];
  };
  EXPECT_CALL(mock, call(Truly(is_expected))).Times(1);

  // Wrapper for mocked callback
  auto cb = [&](auto res) {
    if (res.has_value()) {
      auto &bi = res.value();
      std::cout << "Success: " << bi.hash.data() << std::endl;
    } else {
      std::cout << "Fail: " << res.error() << std::endl;
    }
    std::cout << std::endl;
    mock(res);
  };

  /// @when find of the best common block
  auto lower = finalized;
  auto hint = std::min(local_best, remote_best);
  auto upper = std::min(local_best, remote_best) + 1;
  synchronizer->findCommonBlock(peer_id, lower, upper, hint, std::move(cb));
}

INSTANTIATE_TEST_SUITE_P(
    SynchronizerTest_Success,
    SynchronizerTest,
    Values(  // clang-format off

// common block is not finalized
std::make_tuple(3, 5, 5, 5),   // equal chains, common is best for both
std::make_tuple(3, 5, 10, 10), // equal size of chains, common isn't best
std::make_tuple(3, 5, 10, 15), // remote chain longer, common isn't best
std::make_tuple(3, 5, 5, 15),  // remote chain longer, common is best for local
std::make_tuple(3, 5, 15, 10), // local chain longer, common is not best
std::make_tuple(3, 5, 10, 5),  // local chain longer, common is best for remote

// common block is finalized
std::make_tuple(5, 5, 5, 5),   // equal chains, common is best for both
std::make_tuple(5, 5, 10, 10), // equal size of chains, common isn't best
std::make_tuple(5, 5, 10, 15), // remote chain longer, common isn't best
std::make_tuple(5, 5, 5, 15),  // remote chain longer, common is best for local
std::make_tuple(5, 5, 15, 10), // local chain longer, common is not best
std::make_tuple(5, 5, 10, 5)   // local chain longer, common is best for remote

    ));  // clang-format on
