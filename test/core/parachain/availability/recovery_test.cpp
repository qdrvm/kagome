/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/availability/recovery/recovery_impl.hpp"

#include <gtest/gtest.h>

#include "crypto/random_generator/boost_generator.hpp"
#include "mock/core/application/chain_spec_mock.hpp"
#include "mock/core/authority_discovery/query_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/crypto/session_keys_mock.hpp"
#include "mock/core/network/peer_manager_mock.hpp"
#include "mock/core/network/router_mock.hpp"
#include "mock/core/parachain/availability_store_mock.hpp"
#include "mock/core/runtime/parachain_host_mock.hpp"
#include "parachain/availability/chunks.hpp"
#include "parachain/availability/proof.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::Buffer;
using kagome::application::ChainSpecMock;
using kagome::authority_discovery::QueryMock;
using kagome::blockchain::BlockTreeMock;
using kagome::common::Buffer;
using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::HasherMock;
using kagome::crypto::SessionKeysMock;
using kagome::network::CandidateHash;
using kagome::network::CandidateReceipt;
using kagome::network::Chunk;
using kagome::network::PeerManagerMock;
using kagome::network::PeerState;
using kagome::network::RouterMock;
using kagome::parachain::AvailabilityStoreMock;
using kagome::parachain::CoreIndex;
using kagome::parachain::GroupIndex;
using kagome::parachain::Recovery;
using kagome::parachain::RecoveryImpl;
using kagome::parachain::SessionIndex;
using kagome::parachain::ValidatorId;
using kagome::primitives::AuthorityDiscoveryId;
using kagome::primitives::BlockInfo;
using kagome::runtime::AvailableData;
using kagome::runtime::NodeFeatures;
using kagome::runtime::ParachainHost;
using kagome::runtime::ParachainHostMock;
using kagome::runtime::SessionInfo;

using kagome::parachain::makeTrieProof;
using kagome::parachain::minChunks;
using kagome::parachain::toChunks;

using libp2p::PeerId;
using libp2p::peer::PeerInfo;

using testing::_;
using testing::AnyNumber;
using testing::Args;
using testing::Invoke;
using testing::Return;
using testing::ReturnRef;
using testing::Truly;
using testing::WithArgs;

class RecoveryTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers(soralog::Level::TRACE);
  }

  void prepareAvailableData(size_t data_size) {
    original_available_data.pov.payload.resize(data_size);
    random_generator.fillRandomly(original_available_data.pov.payload);

    original_chunks = toChunks(n_validators, original_available_data).value();
    receipt.descriptor.erasure_encoding_root = makeTrieProof(original_chunks);

    required_chunk_number = minChunks(n_validators).value();
    tolerance_chunk_number = original_chunks.size() - required_chunk_number;

    auto candidate_hash = Recovery::HashedCandidateReceipt{receipt}.getHash();

    EXPECT_CALL(*av_store, getChunk(candidate_hash, 1))
        .WillRepeatedly(Return(original_chunks[1]));
  }

  void SetUp() override {
    session = SessionInfo{};

    chain_spec = std::make_shared<ChainSpecMock>();
    static std::string chain_type{"network"};
    EXPECT_CALL(*chain_spec, chainType()).WillRepeatedly(ReturnRef(chain_type));

    hasher = std::make_shared<HasherMock>();

    block_tree = std::make_shared<BlockTreeMock>();
    ON_CALL(*block_tree, bestBlock()).WillByDefault(Return(best_block));

    parachain_api = std::make_shared<ParachainHostMock>();
    ON_CALL(*parachain_api, session_info(best_block.hash, session_index))
        .WillByDefault(Invoke([&] { return session; }));
    ON_CALL(*parachain_api, node_features(best_block.hash))
        .WillByDefault(Invoke([&] {
          scale::BitVec bits;
          bits.bits.resize(NodeFeatures::FirstUnassigned);
          bits.bits[NodeFeatures::AvailabilityChunkMapping] = true;
          return NodeFeatures{.bits = std::move(bits)};
        }));

    av_store = std::make_shared<AvailabilityStoreMock>();

    query_audi = std::make_shared<QueryMock>();

    router = std::make_shared<RouterMock>();
    router->setReturningMockedProtocols();
    EXPECT_CALL(*router, getFetchAvailableDataProtocol).Times(AnyNumber());
    EXPECT_CALL(*router, getFetchChunkProtocol).Times(AnyNumber());

    auto fetch_available_data = router->getMockedFetchAvailableDataProtocol();
    ON_CALL(*fetch_available_data, doRequest(_, _, _))
        .WillByDefault(WithArgs<0, 1, 2>([&](auto &&...args) {
          fetch_available_data_requests.emplace(
              std::forward<decltype(args)>(args)...);
        }));

    auto fetch_chunk = router->getMockedFetchChunkProtocol();
    ON_CALL(*fetch_chunk, doRequest(_, _, _))
        .WillByDefault(WithArgs<0, 1, 2>([&](auto &&...args) {
          fetch_chunk_requests.emplace(std::forward<decltype(args)>(args)...);
        }));

    auto fetch_chunk_obsolete = router->getMockedFetchChunkProtocolObsolete();
    ON_CALL(*fetch_chunk_obsolete, doRequest(_, _, _))
        .WillByDefault(WithArgs<0, 1, 2>([&](auto &&...args) {
          fetch_chunk_obsolete_requests.emplace(
              std::forward<decltype(args)>(args)...);
        }));

    peer_manager = std::make_shared<PeerManagerMock>();

    session_keys = std::make_shared<SessionKeysMock>();
    ON_CALL(*session_keys, getParaKeyPair(_))
        .WillByDefault(Return(
            std::pair(std::shared_ptr<kagome::crypto::Sr25519Keypair>{}, 1)));

    recovery = std::make_shared<RecoveryImpl>(chain_spec,
                                              hasher,
                                              block_tree,
                                              parachain_api,
                                              av_store,
                                              query_audi,
                                              router,
                                              peer_manager,
                                              session_keys);

    auto &val_group_0 = session.validator_groups.emplace_back();
    for (size_t i = 0; i < n_validators; ++i) {
      auto s = fmt::format("Validator#{:<{}}", i, ValidatorId::size() - 10);
      ValidatorId validator_id =
          ValidatorId::fromSpan(Buffer::fromString(s)).value();

      s = fmt::format("Authority#{:<{}}", i, AuthorityDiscoveryId::size() - 10);
      AuthorityDiscoveryId audi_id =
          AuthorityDiscoveryId::fromSpan(Buffer::fromString(s)).value();

      s = fmt::format("Peer#{}", i);
      PeerId peer_id = operator""_peerid(s.data(), s.size());

      session.validators.push_back(validator_id);
      session.discovery_keys.push_back(audi_id);
      val_group_0.push_back(i);

      ON_CALL(*query_audi, get(audi_id))
          .WillByDefault(Return(PeerInfo{.id = peer_id, .addresses = {}}));
      ON_CALL(*query_audi, get(peer_id)).WillByDefault(Return(audi_id));
      ON_CALL(*peer_manager, getPeerState(peer_id)).WillByDefault(Invoke([&] {
        return std::ref(peer_state);
      }));
    }

    callback = std::make_shared<testing::MockFunction<void(
        std::optional<outcome::result<AvailableData>>)>>();
  }

  void TearDown() override {
    original_chunks.clear();
  }

  BoostRandomGenerator random_generator;

  size_t n_validators = 10;
  size_t required_chunk_number;
  size_t tolerance_chunk_number;
  AvailableData original_available_data{};
  std::vector<kagome::network::ErasureChunk> original_chunks;

  CandidateReceipt receipt;
  BlockInfo best_block{};
  SessionIndex session_index;
  SessionInfo session{};
  PeerState peer_state;

  std::queue<std::tuple<
      PeerId,
      CandidateHash,
      std::function<void(
          outcome::result<kagome::network::FetchAvailableDataResponse>)>>>
      fetch_available_data_requests;

  std::queue<std::tuple<  //
      PeerId,
      kagome::network::FetchChunkRequest,
      std::function<void(
          outcome::result<kagome::network::FetchChunkResponse>)>>>
      fetch_chunk_requests;

  std::queue<std::tuple<
      PeerId,
      kagome::network::FetchChunkRequest,
      std::function<void(
          outcome::result<kagome::network::FetchChunkResponseObsolete>)>>>
      fetch_chunk_obsolete_requests;

  std::shared_ptr<ChainSpecMock> chain_spec;
  std::shared_ptr<HasherMock> hasher;
  std::shared_ptr<BlockTreeMock> block_tree;
  std::shared_ptr<ParachainHostMock> parachain_api;
  std::shared_ptr<AvailabilityStoreMock> av_store;
  std::shared_ptr<QueryMock> query_audi;
  std::shared_ptr<RouterMock> router;
  std::shared_ptr<PeerManagerMock> peer_manager;
  std::shared_ptr<SessionKeysMock> session_keys;

  std::shared_ptr<testing::MockFunction<void(
      std::optional<outcome::result<AvailableData>>)>>
      callback;

  std::shared_ptr<RecoveryImpl> recovery;
};

TEST_F(RecoveryTest, FullFromBakers_NoGroup) {
  prepareAvailableData(2048);

  std::optional<GroupIndex> backing_group = std::nullopt;
  std::optional<CoreIndex> core = std::nullopt;  // value doesn't matter

  recovery->recover(
      receipt, session_index, backing_group, core, callback->AsStdFunction());

  // No one fetch available data request was sent
  ASSERT_TRUE(fetch_available_data_requests.empty());
}

TEST_F(RecoveryTest, FullFromBakers_Success) {
  prepareAvailableData(2048);

  std::optional<GroupIndex> backing_group = 0;   // any non nullopt value
  std::optional<CoreIndex> core = std::nullopt;  // value doesn't matter

  std::optional<outcome::result<AvailableData>> available_data_res_opt;

  EXPECT_CALL(*callback, Call(_))
      .WillOnce(WithArgs<0>(
          [&](std::optional<outcome::result<AvailableData>> x) mutable {
            available_data_res_opt = std::move(x);
          }));

  recovery->recover(
      receipt, session_index, backing_group, core, callback->AsStdFunction());

  while (not fetch_available_data_requests.empty()) {
    auto [peer_id, req, cb] = std::move(fetch_available_data_requests.front());
    fetch_available_data_requests.pop();
    cb(original_available_data);
  }

  testing::Mock::VerifyAndClear(callback.get());

  ASSERT_TRUE(available_data_res_opt.has_value());
  ASSERT_OUTCOME_SUCCESS(available_data, available_data_res_opt.value());
  ASSERT_EQ(available_data, original_available_data);
}

TEST_F(RecoveryTest, SystematicChunks_NoCore) {
  prepareAvailableData(2048);

  peer_state.req_chunk_version = kagome::network::ReqChunkVersion::V2;
  std::optional<GroupIndex> backing_group{};  // nullopt to skip full recovery
  std::optional<CoreIndex> core = std::nullopt;

  std::optional<outcome::result<AvailableData>> available_data_res_opt;

  EXPECT_CALL(*callback, Call(_))
      .WillOnce(WithArgs<0>(
          [&](std::optional<outcome::result<AvailableData>> x) mutable {
            available_data_res_opt = std::move(x);
          }));

  recovery->recover(
      receipt, session_index, backing_group, core, callback->AsStdFunction());

  // Full recovery from bakers is skipped, 'cause backing_group is unknown
  ASSERT_TRUE(fetch_available_data_requests.empty());

  // Trying to do systematic chunks recovery
  while (not fetch_chunk_requests.empty()) {
    auto &[peer_id, req, cb] = fetch_chunk_requests.front();
    const auto &ec_chunk = original_chunks[req.chunk_index];
    Chunk chunk{
        .data = ec_chunk.chunk,
        .chunk_index = ec_chunk.index,
        .proof = ec_chunk.proof,
    };
    cb(chunk);
    fetch_chunk_requests.pop();
  }

  testing::Mock::VerifyAndClear(callback.get());

  ASSERT_TRUE(available_data_res_opt.has_value());
  ASSERT_OUTCOME_SUCCESS(available_data, available_data_res_opt.value());
  ASSERT_EQ(available_data, original_available_data);
}

TEST_F(RecoveryTest, SystematicChunks_Success) {
  prepareAvailableData(2048);

  peer_state.req_chunk_version = kagome::network::ReqChunkVersion::V2;
  std::optional<GroupIndex> backing_group{};  // nullopt to skip full recovery
  std::optional<CoreIndex> core = 0;

  std::optional<outcome::result<AvailableData>> available_data_res_opt;

  EXPECT_CALL(*callback, Call(_))
      .WillOnce(WithArgs<0>(
          [&](std::optional<outcome::result<AvailableData>> x) mutable {
            available_data_res_opt = std::move(x);
          }));

  recovery->recover(
      receipt, session_index, backing_group, core, callback->AsStdFunction());

  // Full recovery from bakers is skipped, 'cause backing_group is unknown
  ASSERT_TRUE(fetch_available_data_requests.empty());

  // Trying to do systematic chunks recovery
  while (not fetch_chunk_requests.empty()) {
    auto &[peer_id, req, cb] = fetch_chunk_requests.front();
    const auto &ec_chunk = original_chunks[req.chunk_index];
    Chunk chunk{
        .data = ec_chunk.chunk,
        .chunk_index = ec_chunk.index,
        .proof = ec_chunk.proof,
    };
    cb(chunk);
    fetch_chunk_requests.pop();
  }

  testing::Mock::VerifyAndClear(callback.get());

  ASSERT_TRUE(available_data_res_opt.has_value());
  ASSERT_OUTCOME_SUCCESS(available_data, available_data_res_opt.value());
  ASSERT_EQ(available_data, original_available_data);
}

TEST_F(RecoveryTest, RegularChunks_Success) {
  prepareAvailableData(2048);

  peer_state.req_chunk_version = kagome::network::ReqChunkVersion::V2;
  std::optional<GroupIndex> backing_group{};  // nullopt to skip full recovery
  std::optional<CoreIndex> core = 0;

  std::optional<outcome::result<AvailableData>> available_data_res_opt;

  EXPECT_CALL(*callback, Call(_))
      .WillOnce(WithArgs<0>(
          [&](std::optional<outcome::result<AvailableData>> x) mutable {
            available_data_res_opt = std::move(x);
          }));

  recovery->recover(
      receipt, session_index, backing_group, core, callback->AsStdFunction());

  // Full recovery from bakers is skipped, 'cause backing_group is unknown
  ASSERT_TRUE(fetch_available_data_requests.empty());

  // Trying to do systematic chunks recovery, but one chunk unavailable
  while (not fetch_chunk_requests.empty()) {
    auto &[peer_id, req, cb] = fetch_chunk_requests.front();
    const auto &ec_chunk = original_chunks[req.chunk_index];
    if (ec_chunk.index != 0) {
      Chunk chunk{
          .data = ec_chunk.chunk,
          .chunk_index = ec_chunk.index,
          .proof = ec_chunk.proof,
      };
      cb(chunk);
    } else {
      cb(kagome::network::Empty{});
    }
    fetch_chunk_requests.pop();
  }

  testing::Mock::VerifyAndClear(callback.get());

  ASSERT_TRUE(available_data_res_opt.has_value());
  ASSERT_OUTCOME_SUCCESS(available_data, available_data_res_opt.value());
  ASSERT_EQ(available_data, original_available_data);
}

TEST_F(RecoveryTest, CorruptedChunk) {
  prepareAvailableData(2048);

  peer_state.req_chunk_version = kagome::network::ReqChunkVersion::V2;
  std::optional<GroupIndex> backing_group = 0;
  std::optional<CoreIndex> core = 0;

  std::optional<outcome::result<AvailableData>> available_data_res_opt;

  EXPECT_CALL(*callback, Call(_))
      .WillOnce(WithArgs<0>(
          [&](std::optional<outcome::result<AvailableData>> x) mutable {
            available_data_res_opt = std::move(x);
          }));

  recovery->recover(
      receipt, session_index, backing_group, core, callback->AsStdFunction());

  // Trying to recover full from bakers, but all return no-data
  while (not fetch_available_data_requests.empty()) {
    auto &[peer_id, req, cb] = fetch_available_data_requests.front();
    cb(kagome::network::Empty{});
    fetch_available_data_requests.pop();
  }

  // Trying to do chunks recovery, but chunks
  // tolerated number are received corrupted
  while (not fetch_chunk_requests.empty()) {
    auto &[peer_id, req, cb] = fetch_chunk_requests.front();
    const auto &ec_chunk = original_chunks[req.chunk_index];
    Chunk chunk{
        .data = ec_chunk.chunk,
        .chunk_index = ec_chunk.index,
        .proof = ec_chunk.proof,
    };
    if (chunk.chunk_index < tolerance_chunk_number) {
      chunk.data = {"corrupted chunk"_bytes};
    }
    cb(chunk);
    fetch_chunk_requests.pop();
  }

  testing::Mock::VerifyAndClear(callback.get());

  ASSERT_TRUE(available_data_res_opt.has_value());
}

TEST_F(RecoveryTest, InsufficientChunks) {
  prepareAvailableData(2048);

  peer_state.req_chunk_version = kagome::network::ReqChunkVersion::V2;
  std::optional<GroupIndex> backing_group = 0;
  std::optional<CoreIndex> core = 0;

  std::optional<outcome::result<AvailableData>> available_data_res_opt;

  EXPECT_CALL(*callback, Call(_))
      .WillOnce(WithArgs<0>(
          [&](std::optional<outcome::result<AvailableData>> x) mutable {
            available_data_res_opt = std::move(x);
          }));

  recovery->recover(
      receipt, session_index, backing_group, core, callback->AsStdFunction());

  // Trying to recover full from bakers, but all return no-data
  while (not fetch_available_data_requests.empty()) {
    auto &[peer_id, req, cb] = fetch_available_data_requests.front();
    cb(kagome::network::Empty{});
    fetch_available_data_requests.pop();
  }

  // Trying to do chunks recovery, but chunks less than required number are
  // received
  while (not fetch_chunk_requests.empty()) {
    auto &[peer_id, req, cb] = fetch_chunk_requests.front();
    const auto &ec_chunk = original_chunks[req.chunk_index];
    if (ec_chunk.index < required_chunk_number - 1) {
      Chunk chunk{
          .data = ec_chunk.chunk,
          .chunk_index = ec_chunk.index,
          .proof = ec_chunk.proof,
      };
      cb(chunk);
    } else {
      cb(kagome::network::Empty{});
    }
    fetch_chunk_requests.pop();
  }

  testing::Mock::VerifyAndClear(callback.get());

  ASSERT_FALSE(available_data_res_opt.has_value());
}

TEST_F(RecoveryTest, DelayedChunks) {
  prepareAvailableData(2048);

  peer_state.req_chunk_version = kagome::network::ReqChunkVersion::V2;
  std::optional<GroupIndex> backing_group = 0;
  std::optional<CoreIndex> core = 0;

  std::optional<outcome::result<AvailableData>> available_data_res_opt;

  recovery->recover(
      receipt, session_index, backing_group, core, callback->AsStdFunction());

  // Trying to recover full from bakers, but all return no-data
  while (not fetch_available_data_requests.empty()) {
    auto &[peer_id, req, cb] = fetch_available_data_requests.front();
    cb(kagome::network::Empty{});
    fetch_available_data_requests.pop();
  }

  size_t handled_counter = 0;

  EXPECT_CALL(*callback, Call(_)).Times(0);

  // Trying to do chunks recovery, but handled chunks
  // less than required number to emulate of delay
  while (not fetch_chunk_requests.empty()) {
    auto &[peer_id, req, cb] = fetch_chunk_requests.front();
    const auto &ec_chunk = original_chunks[req.chunk_index];
    Chunk chunk{
        .data = ec_chunk.chunk,
        .chunk_index = ec_chunk.index,
        .proof = ec_chunk.proof,
    };
    cb(chunk);
    fetch_chunk_requests.pop();

    // Stop responding for required-1 to emulate of delay)
    if (++handled_counter == required_chunk_number - 1) {
      break;
    }
  }

  // Actually stopped for emulation of delay
  ASSERT_EQ(handled_counter, required_chunk_number - 1);

  // Some requests still active (delayed)
  ASSERT_FALSE(fetch_chunk_requests.empty());

  // Available data still is not reconstructed
  ASSERT_FALSE(available_data_res_opt.has_value());

  testing::Mock::VerifyAndClear(callback.get());

  EXPECT_CALL(*callback, Call(_))
      .WillOnce(WithArgs<0>(
          [&](std::optional<outcome::result<AvailableData>> x) mutable {
            available_data_res_opt = std::move(x);
          }));

  // After continue data should be completely reconstructed
  while (not fetch_chunk_requests.empty()) {
    auto &[peer_id, req, cb] = fetch_chunk_requests.front();
    const auto &ec_chunk = original_chunks[req.chunk_index];
    Chunk chunk{
        .data = ec_chunk.chunk,
        .chunk_index = ec_chunk.index,
        .proof = ec_chunk.proof,
    };
    cb(chunk);
    fetch_chunk_requests.pop();
  }

  testing::Mock::VerifyAndClear(callback.get());

  ASSERT_TRUE(available_data_res_opt.has_value());
}

TEST_F(RecoveryTest, DuplicateChunk) {
  prepareAvailableData(2048);

  peer_state.req_chunk_version = kagome::network::ReqChunkVersion::V2;
  std::optional<GroupIndex> backing_group = 0;
  std::optional<CoreIndex> core = 0;

  std::optional<outcome::result<AvailableData>> available_data_res_opt;

  EXPECT_CALL(*callback, Call(_))
      .WillOnce(WithArgs<0>(
          [&](std::optional<outcome::result<AvailableData>> x) mutable {
            available_data_res_opt = std::move(x);
          }));

  recovery->recover(
      receipt, session_index, backing_group, core, callback->AsStdFunction());

  // Trying to recover full from bakers, but all return no-data
  while (not fetch_available_data_requests.empty()) {
    auto &[peer_id, req, cb] = fetch_available_data_requests.front();
    cb(kagome::network::Empty{});
    fetch_available_data_requests.pop();
  }

  // Trying to do chunks recovery, but for tolerated number
  // of first requests returned one the same chunk
  while (not fetch_chunk_requests.empty()) {
    auto &[peer_id, req, cb] = fetch_chunk_requests.front();
    const auto &ec_chunk =
        original_chunks[req.chunk_index < tolerance_chunk_number
                            ? 0
                            : req.chunk_index];
    Chunk chunk{
        .data = ec_chunk.chunk,
        .chunk_index = ec_chunk.index,
        .proof = ec_chunk.proof,
    };
    cb(chunk);
    fetch_chunk_requests.pop();
  }

  testing::Mock::VerifyAndClear(callback.get());

  ASSERT_TRUE(available_data_res_opt.has_value());
}

TEST_F(RecoveryTest, FailureHandling) {
  prepareAvailableData(2048);

  peer_state.req_chunk_version = kagome::network::ReqChunkVersion::V2;
  std::optional<GroupIndex> backing_group = 0;
  std::optional<CoreIndex> core = 0;

  std::optional<outcome::result<AvailableData>> available_data_res_opt;

  EXPECT_CALL(*callback, Call(_))
      .WillOnce(WithArgs<0>(
          [&](std::optional<outcome::result<AvailableData>> x) mutable {
            available_data_res_opt = std::move(x);
          }));

  recovery->recover(
      receipt, session_index, backing_group, core, callback->AsStdFunction());

  // Trying to recover full from bakers, but all return no-data
  while (not fetch_available_data_requests.empty()) {
    auto &[peer_id, req, cb] = fetch_available_data_requests.front();
    cb(kagome::network::Empty{});
    fetch_available_data_requests.pop();
  }

  size_t handled_counter = 0;

  // Trying to do chunks recovery, but tolerated number of requests are failed
  while (not fetch_chunk_requests.empty()) {
    auto &[peer_id, req, cb] = fetch_chunk_requests.front();
    if (handled_counter++ < tolerance_chunk_number) {
      cb(kagome::network::ProtocolError::GONE);
    } else {
      const auto &ec_chunk = original_chunks[req.chunk_index];
      Chunk chunk{
          .data = ec_chunk.chunk,
          .chunk_index = ec_chunk.index,
          .proof = ec_chunk.proof,
      };
      cb(chunk);
    }
    fetch_chunk_requests.pop();
  }

  testing::Mock::VerifyAndClear(callback.get());

  ASSERT_TRUE(available_data_res_opt.has_value());
}

TEST_F(RecoveryTest, LargeScaleRecovery) {
  prepareAvailableData(2 << 20);  // 2Mb

  std::optional<GroupIndex> backing_group = 0;   // any non nullopt value
  std::optional<CoreIndex> core = std::nullopt;  // value doesn't matter

  std::optional<outcome::result<AvailableData>> available_data_res_opt;

  EXPECT_CALL(*callback, Call(_))
      .WillOnce(WithArgs<0>(
          [&](std::optional<outcome::result<AvailableData>> x) mutable {
            available_data_res_opt = std::move(x);
          }));

  recovery->recover(
      receipt, session_index, backing_group, core, callback->AsStdFunction());

  // We must not try to obtain big data from backers
  EXPECT_TRUE(fetch_available_data_requests.empty());

  // Trying to do systematic chunks recovery
  while (not fetch_chunk_requests.empty()) {
    auto &[peer_id, req, cb] = fetch_chunk_requests.front();
    const auto &ec_chunk = original_chunks[req.chunk_index];
    Chunk chunk{
        .data = ec_chunk.chunk,
        .chunk_index = ec_chunk.index,
        .proof = ec_chunk.proof,
    };
    cb(chunk);
    fetch_chunk_requests.pop();
  }

  testing::Mock::VerifyAndClear(callback.get());

  ASSERT_TRUE(available_data_res_opt.has_value());
  ASSERT_OUTCOME_SUCCESS(available_data, available_data_res_opt.value());
  ASSERT_EQ(available_data, original_available_data);
}
