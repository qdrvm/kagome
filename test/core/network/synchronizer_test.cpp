/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include <mock/libp2p/basic/scheduler_mock.hpp>
#include <stdexcept>

#include "common/main_thread_pool.hpp"
#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_storage_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/clock/clock_mock.hpp"
#include "mock/core/consensus/grandpa/environment_mock.hpp"
#include "mock/core/consensus/timeline/block_appender_mock.hpp"
#include "mock/core/consensus/timeline/block_executor_mock.hpp"
#include "mock/core/consensus/timeline/slots_util_mock.hpp"
#include "mock/core/network/peer_manager_mock.hpp"
#include "mock/core/network/protocols/sync_protocol_mock.hpp"
#include "mock/core/network/router_mock.hpp"
#include "mock/core/runtime/module_factory_mock.hpp"
#include "mock/core/runtime/runtime_properties_cache_mock.hpp"
#include "mock/core/storage/generic_storage_mock.hpp"
#include "mock/core/storage/spaced_storage_mock.hpp"
#include "mock/core/storage/trie/trie_storage_backend_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "mock/core/storage/trie_pruner/trie_pruner_mock.hpp"
#include "network/impl/synchronizer_impl.hpp"
#include "primitives/common.hpp"
#include "primitives/extrinsic_root.hpp"
#include "testutil/lazy.hpp"
#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"
#include "utils/map_entry.hpp"

using kagome::entry;
using kagome::blockchain::BlockTreeError;
using kagome::clock::SystemClockMock;
using kagome::consensus::SlotsUtil;
using kagome::consensus::SlotsUtilMock;
using kagome::consensus::babe::BabeBlockHeader;
using kagome::crypto::HasherImpl;
using kagome::network::BlockAttribute;
using kagome::network::BlocksRequest;
using kagome::network::BlocksResponse;
using kagome::network::Direction;
using kagome::network::PeerManagerMock;
using kagome::network::PeerState;
using kagome::network::SyncProtocolMock;
using kagome::primitives::BlockBody;
using kagome::primitives::kBabeEngineId;
using kagome::primitives::PreRuntime;
using kagome::primitives::Seal;
using libp2p::PeerId;

using namespace kagome;
using consensus::BlockExecutorMock;
using consensus::BlockHeaderAppenderMock;
using namespace consensus::grandpa;
using namespace storage;
using application::AppStateManagerMock;
using std::chrono_literals::operator""ms;
using common::MainThreadPool;
using consensus::Timeline;
using network::Synchronizer;
using primitives::BlockData;
using primitives::BlockHash;
using primitives::BlockHeader;
using primitives::BlockInfo;
using primitives::BlockNumber;

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
    genesis_.updateHash(*hasher);
    db_blocks_.emplace(genesis_.hash(), genesis_);
    best_ = genesis_.blockInfo();
    finalized_ = best_;
    EXPECT_CALL(*block_tree, bestBlock()).WillRepeatedly([this] {
      return best_;
    });
    EXPECT_CALL(*block_tree, has(_))
        .WillRepeatedly([this](const BlockHash &hash) {
          return db_blocks_.contains(hash);
        });
    EXPECT_CALL(*block_tree, getBlockHeader(_))
        .WillRepeatedly(
            [this](const BlockHash &hash) -> outcome::result<BlockHeader> {
              if (auto block = entry(db_blocks_, hash)) {
                return *block;
              }
              return BlockTreeError::HEADER_NOT_FOUND;
            });
    EXPECT_CALL(*block_tree, getLastFinalized()).WillRepeatedly([this] {
      return finalized_;
    });

    EXPECT_CALL(*clock_, now())
        .WillRepeatedly(Return(SystemClockMock::TimePoint{}));
    EXPECT_CALL(*slots_util_, timeToSlot(_)).WillRepeatedly(Return(100));

    EXPECT_CALL(*app_state_manager, atLaunch(_)).WillRepeatedly(Return());
    EXPECT_CALL(*app_state_manager, atShutdown(_)).WillRepeatedly(Return());

    EXPECT_CALL(*router, getSyncProtocol())
        .WillRepeatedly(Return(sync_protocol));

    EXPECT_CALL(*peer_manager_, enumeratePeerState(_))
        .WillRepeatedly([this](PeerManagerMock::PeersCallback cb) {
          PeerState state;
          state.best_block.number = peer_best_;
          cb(peer_id, state);
        });

    EXPECT_CALL(*scheduler, scheduleImpl(_, _, _)).Times(AnyNumber());

    EXPECT_CALL(app_config, syncMethod())
        .WillOnce(Return(application::SyncMethod::Full));
    EXPECT_CALL(app_config, maxParallelDownloads()).WillOnce(Return(1));

    auto state_pruner =
        std::make_shared<kagome::storage::trie_pruner::TriePrunerMock>();

    main_thread_pool = std::make_shared<MainThreadPool>(
        watchdog, std::make_shared<boost::asio::io_context>());

    synchronizer = std::make_shared<network::SynchronizerImpl>(
        app_config,
        *app_state_manager,
        block_tree,
        clock_,
        testutil::sptr_to_lazy<SlotsUtil>(slots_util_),
        block_appender,
        block_executor,
        trie_node_db,
        storage,
        state_pruner,
        router,
        peer_manager_,
        scheduler,
        hasher,
        chain_sub_engine,
        testutil::sptr_to_lazy<Timeline>(timeline),
        nullptr,
        grandpa_environment,
        *main_thread_pool,
        block_storage);
  }

  void TearDown() override {
    watchdog->stop();
  }

  BlockHeader makeBlock(const BlockHeader &parent) {
    BlockHeader header;
    header.number = parent.number + 1;
    header.parent_hash = parent.hash();
    header.extrinsics_root = extrinsicRoot(body_);
    BabeBlockHeader pre{.slot_number = header.number};
    header.digest.emplace_back(PreRuntime{
        kBabeEngineId,
        Buffer{::kagome::scale::encode(pre).value()},
    });
    header.digest.emplace_back(Seal{});
    header.updateHash(*hasher);
    return header;
  }

  auto expectRequest(const BlocksRequest &request) {
    auto cb_out = std::make_shared<std::optional<SyncProtocolMock::Cb>>();
    EXPECT_CALL(*sync_protocol, request(_, request, _))
        .WillOnce([cb_out](const PeerId &,
                           BlocksRequest,
                           SyncProtocolMock::Cb cb) { *cb_out = cb; });
    return [this, request, cb_out](std::vector<BlockHeader> blocks) {
      auto &cb = cb_out->value();
      if (blocks.empty()) {
        cb(std::errc::not_supported);
        return;
      }
      BlocksResponse response;
      for (auto &header : blocks) {
        auto &block = response.blocks.emplace_back();
        block.hash = header.hash();
        if (has(request.fields, BlockAttribute::HEADER)) {
          block.header = header;
        }
        if (has(request.fields, BlockAttribute::BODY)) {
          block.body = body_;
        }
      }
      cb(response);
    };
  }
  auto expectBodyRequest(const BlockHeader &block) {
    return expectRequest({
        .fields = BlockAttribute::HEADER | BlockAttribute::BODY,
        .from = block.hash(),
        .direction = Direction::ASCENDING,
    });
  }
  auto expectGapRequest(const BlockHeader &block) {
    return expectRequest({
        .fields = BlockAttribute::HEADER | BlockAttribute::JUSTIFICATION,
        .from = block.hash(),
        .direction = Direction::DESCENDING,
    });
  }
  auto expectRangeRequest(BlockNumber number) {
    return expectRequest({
        .fields = BlockAttribute::HEADER | BlockAttribute::JUSTIFICATION,
        .from = number,
        .direction = Direction::ASCENDING,
    });
  }

  application::AppConfigurationMock app_config;
  std::shared_ptr<application::AppStateManagerMock> app_state_manager =
      std::make_shared<application::AppStateManagerMock>();
  std::shared_ptr<blockchain::BlockTreeMock> block_tree =
      std::make_shared<blockchain::BlockTreeMock>();
  std::shared_ptr<SystemClockMock> clock_ = std::make_shared<SystemClockMock>();
  std::shared_ptr<SlotsUtilMock> slots_util_ =
      std::make_shared<SlotsUtilMock>();
  std::shared_ptr<BlockHeaderAppenderMock> block_appender =
      std::make_shared<BlockHeaderAppenderMock>();
  std::shared_ptr<BlockExecutorMock> block_executor =
      std::make_shared<BlockExecutorMock>();
  std::shared_ptr<trie::TrieStorageBackendMock> trie_node_db =
      std::make_shared<trie::TrieStorageBackendMock>();
  std::shared_ptr<trie::TrieStorageMock> storage =
      std::make_shared<trie::TrieStorageMock>();
  std::shared_ptr<SyncProtocolMock> sync_protocol =
      std::make_shared<SyncProtocolMock>();
  std::shared_ptr<network::RouterMock> router =
      std::make_shared<network::RouterMock>();
  std::shared_ptr<PeerManagerMock> peer_manager_ =
      std::make_shared<PeerManagerMock>();
  std::shared_ptr<libp2p::basic::SchedulerMock> scheduler =
      std::make_shared<libp2p::basic::SchedulerMock>();
  std::shared_ptr<HasherImpl> hasher = std::make_shared<HasherImpl>();
  primitives::events::ChainSubscriptionEnginePtr chain_sub_engine =
      std::make_shared<primitives::events::ChainSubscriptionEngine>();
  std::shared_ptr<Timeline> timeline;
  std::shared_ptr<BufferStorageMock> buffer_storage =
      std::make_shared<BufferStorageMock>();
  std::shared_ptr<EnvironmentMock> grandpa_environment =
      std::make_shared<EnvironmentMock>();
  std::shared_ptr<blockchain::BlockStorageMock> block_storage =
      std::make_shared<blockchain::BlockStorageMock>();

  std::shared_ptr<Watchdog> watchdog =
      std::make_shared<Watchdog>(std::chrono::milliseconds(1));
  std::shared_ptr<MainThreadPool> main_thread_pool;

  std::shared_ptr<network::SynchronizerImpl> synchronizer;

  libp2p::peer::PeerId peer_id = ""_peerid;

  std::tuple<std::vector<BlockInfo>, std::vector<BlockInfo>> generateChains(
      primitives::BlockNumber finalized,
      primitives::BlockNumber common,
      primitives::BlockNumber local_best,
      primitives::BlockNumber remote_best);

  BlockHeader genesis_;
  BlockBody body_;
  BlockInfo best_;
  BlockInfo finalized_;
  std::unordered_map<BlockHash, BlockHeader> db_blocks_;
  BlockNumber peer_best_ = 0;
};

// Imitates call getBlockHeader based on generated local blockchain
ACTION_P(blockTree_getBlockHeader, local_blocks) {
  auto &hash = arg0;
  std::cout << "GetHeader: " << hash.data() << ", ";
  for (BlockNumber block_number = 0; block_number < local_blocks.size();
       ++block_number) {
    if (local_blocks[block_number].hash == hash) {
      const auto &block_info = local_blocks[block_number];
      std::cout << "Result: " << block_info.hash.data() << std::endl;
      return BlockHeader{block_info.number, {}, {}, {}, {}};
    }
  }
  std::cout << "Result: not found" << std::endl;
  return boost::system::error_code{};
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
    response.blocks.emplace_back(
        BlockData{.hash = bi->hash, .header = {{bi->number, {}, {}, {}, {}}}});
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
      EXPECT_CALL(*block_tree, getBestContaining(_))
          .WillRepeatedly(testing::Return(b));
    }
  }
  std::cout << std::endl;

  EXPECT_CALL(*block_tree, getBlockHeader(_))
      .WillRepeatedly(blockTree_getBlockHeader(local_blocks));
  EXPECT_CALL(*block_tree, has(_))
      .WillRepeatedly([this](const BlockHash &hash) {
        return block_tree->getBlockHeader(hash).has_value();
      });

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

TEST_F(SynchronizerTest, Attached) {
  auto block_1 = makeBlock(genesis_);

  auto reply_body = expectBodyRequest(block_1);
  synchronizer->onBlockAnnounce(block_1, peer_id);

  EXPECT_CALL(*block_executor, applyBlock(_, _, _));
  reply_body({block_1});
}

TEST_F(SynchronizerTest, Detached) {
  auto block_1 = makeBlock(genesis_);
  auto block_2 = makeBlock(block_1);
  auto block_3 = makeBlock(block_2);

  auto reply_gap_2 = expectGapRequest(block_2);
  synchronizer->onBlockAnnounce(block_3, peer_id);

  auto reply_gap_1 = expectGapRequest(block_1);
  reply_gap_2({block_2});

  expectBodyRequest(block_1);
  reply_gap_1({block_1});
}

TEST_F(SynchronizerTest, Range) {
  auto block_1 = makeBlock(genesis_);
  auto block_2 = makeBlock(block_1);

  auto reply_range_1 = expectRangeRequest(0);
  peer_best_ = block_2.number;
  synchronizer->addPeerKnownBlockInfo(block_2.blockInfo(), peer_id);

  auto reply_body = expectBodyRequest(block_1);
  reply_range_1({genesis_, block_1});

  expectRangeRequest(1);
  reply_body({});
}
