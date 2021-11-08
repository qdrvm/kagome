/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/system/impl/system_api_impl.hpp"

#include <gtest/gtest.h>

#include "mock/core/application/chain_spec_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/consensus/babe/babe_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/network/peer_manager_mock.hpp"
#include "mock/core/runtime/account_nonce_api_mock.hpp"
#include "mock/core/transaction_pool/transaction_pool_mock.hpp"
#include "scale/scale.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::api::SystemApi;
using kagome::api::SystemApiImpl;
using kagome::application::ChainSpecMock;
using kagome::blockchain::BlockTreeMock;
using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::consensus::babe::BabeMock;
using kagome::crypto::HasherMock;
using kagome::network::PeerManagerMock;
using kagome::primitives::Transaction;
using kagome::runtime::AccountNonceApiMock;
using kagome::transaction_pool::TransactionPoolMock;

using testing::_;
using testing::Return;

class SystemApiTest : public ::testing::Test {
 public:
  void SetUp() {
    chain_spec_mock_ = std::make_shared<ChainSpecMock>();
    babe_mock_ = std::make_shared<BabeMock>();
    peer_manager_mock_ = std::make_shared<PeerManagerMock>();
    transaction_pool_mock_ = std::make_shared<TransactionPoolMock>();
    block_tree_mock_ = std::make_shared<BlockTreeMock>();
    account_nonce_api_mock_ = std::make_shared<AccountNonceApiMock>();
    hasher_mock_ = std::make_shared<HasherMock>();

    system_api_ = std::make_unique<SystemApiImpl>(chain_spec_mock_,
                                                  babe_mock_,
                                                  peer_manager_mock_,
                                                  account_nonce_api_mock_,
                                                  transaction_pool_mock_,
                                                  block_tree_mock_,
                                                  hasher_mock_);
  }

 protected:
  std::unique_ptr<SystemApi> system_api_;

  std::shared_ptr<ChainSpecMock> chain_spec_mock_;
  std::shared_ptr<BabeMock> babe_mock_;
  std::shared_ptr<PeerManagerMock> peer_manager_mock_;
  std::shared_ptr<TransactionPoolMock> transaction_pool_mock_;
  std::shared_ptr<BlockTreeMock> block_tree_mock_;
  std::shared_ptr<AccountNonceApiMock> account_nonce_api_mock_;
  std::shared_ptr<HasherMock> hasher_mock_;

  // Alice's account from subkey
  static constexpr auto kSs58Account =
      "5GrwvaEF5zXb26Fz9rcQpDWS57CtERHpNehXCPcNoHGKutQY";
  static inline kagome::crypto::Sr25519PublicKey kAccountId{
      kagome::common::Blob<32>(std::array<uint8_t, 32>{
          0xd4, 0x35, 0x93, 0xc7, 0x15, 0xfd, 0xd3, 0x1c, 0x61, 0x14, 0x1a,
          0xbd, 0x04, 0xa9, 0x9f, 0xd6, 0x82, 0x2c, 0x85, 0x58, 0x85, 0x4c,
          0xcd, 0xe3, 0x9a, 0x56, 0x84, 0xe7, 0xa5, 0x6d, 0xa2, 0x7d})};
};

/**
 * @given an account id and no pending txs from that account
 * @when querying the account nonce
 * @then the nonce is equal to the value returned from runtime
 */
TEST_F(SystemApiTest, GetNonceNoPendingTxs) {
  constexpr auto kInitialNonce = 42;

  EXPECT_CALL(*block_tree_mock_, deepestLeaf())
      .WillOnce(Return(kagome::primitives::BlockInfo{1, "block1"_hash256}));
  EXPECT_CALL(*account_nonce_api_mock_,
              account_nonce("block1"_hash256, kAccountId))
      .WillOnce(Return(kInitialNonce));
  auto hash_preimage = Buffer{}.put("SS58PRE").putUint8(42).put(kAccountId);
  EXPECT_CALL(*hasher_mock_,
              blake2b_512(gsl::span<const uint8_t>(hash_preimage)))
      .WillOnce(Return(kagome::common::Hash512{{'\035', '!'}}));
  EXPECT_CALL(*transaction_pool_mock_, getReadyTransactions());

  EXPECT_OUTCOME_TRUE(nonce, system_api_->getNonceFor(kSs58Account))
  ASSERT_EQ(nonce, kInitialNonce);
}

/**
 * @given an account id and pending txs from that account
 * @when querying the account nonce
 * @then the nonce is equal to the value returned from runtime PLUS the number
 * of txs from the account
 */
TEST_F(SystemApiTest, GetNonceWithPendingTxs) {
  constexpr auto kInitialNonce = 42;

  EXPECT_CALL(*block_tree_mock_, deepestLeaf())
  .WillOnce(Return(kagome::primitives::BlockInfo{1, "block1"_hash256}));
  EXPECT_CALL(*account_nonce_api_mock_,
              account_nonce("block1"_hash256, kAccountId))
      .WillOnce(Return(kInitialNonce));
  auto hash_preimage = Buffer{}.put("SS58PRE").putUint8(42).put(kAccountId);
  EXPECT_CALL(*hasher_mock_,
              blake2b_512(gsl::span<const uint8_t>(hash_preimage)))
      .WillOnce(Return(kagome::common::Hash512{{'\035', '!'}}));

  constexpr auto kReadyTxNum = 5;
  std::array<std::vector<uint8_t>, kReadyTxNum> encoded_nonces;
  std::map<Transaction::Hash, std::shared_ptr<Transaction>> ready_txs;
  for (size_t i = 0; i < kReadyTxNum; i++) {
    EXPECT_OUTCOME_TRUE(enc_nonce,
                        scale::encode(kAccountId, kInitialNonce + i))
    encoded_nonces[i] = std::move(enc_nonce);
    ready_txs[Hash256{{static_cast<uint8_t>(i)}}] =
        std::make_shared<Transaction>(
            Transaction{.provides = {encoded_nonces[i]}});
  }

  EXPECT_CALL(*transaction_pool_mock_, getReadyTransactions())
      .WillOnce(Return(ready_txs));

  EXPECT_OUTCOME_TRUE(nonce, system_api_->getNonceFor(kSs58Account));
  ASSERT_EQ(nonce, kInitialNonce + kReadyTxNum);
}
