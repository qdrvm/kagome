/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/mp_utils.hpp"
#include "consensus/validation/babe_block_validator.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/crypto/sr25519_provider_mock.hpp"
#include "mock/core/crypto/vrf_provider_mock.hpp"
#include "mock/core/runtime/tagged_transaction_queue_mock.hpp"
#include "mock/libp2p/crypto/random_generator_mock.hpp"
#include "scale/scale.hpp"
#include "testutil/outcome.hpp"
#include "testutil/primitives/mp_utils.hpp"

using namespace kagome;
using namespace blockchain;
using namespace consensus;
using namespace runtime;
using namespace common;
using namespace crypto;
using namespace libp2p::crypto::random;

using kagome::primitives::Authority;
using kagome::primitives::AuthorityIndex;
using kagome::primitives::Block;
using kagome::primitives::BlockBody;
using kagome::primitives::BlockHeader;
using kagome::primitives::ConsensusEngineId;
using kagome::primitives::Digest;
using kagome::primitives::Extrinsic;
using kagome::primitives::Invalid;
using kagome::primitives::PreRuntime;
using kagome::primitives::Valid;

using testing::_;
using testing::Return;
using testing::ReturnRef;

using testutil::createHash256;

namespace sr25519_constants = kagome::crypto::constants::sr25519;

class BlockValidatorTest : public testing::Test {
 public:
  const ConsensusEngineId kEngineId =
      primitives::ConsensusEngineId::fromString("BABE").value();
  /**
   * Add a signature to the block
   * @param block to seal
   * @param block_hash - hash of the block to be sealed
   * @return seal, which was produced, @and public key, which was generated
   */
  std::pair<Seal, SR25519PublicKey> sealBlock(Block &block,
                                              Hash256 block_hash) const {
    // generate a new keypair
    SR25519PublicKey public_key{};
    public_key.fill(8);

    SR25519Signature sr25519_signature{};
    sr25519_signature.fill(0);

    // seal the block
    Seal seal{sr25519_signature};
    common::Buffer encoded_seal{scale::encode(seal).value()};
    block.header.digest.push_back(
        kagome::primitives::Seal{{kEngineId, encoded_seal}});

    return {seal, public_key};
  }

  // fields for validator
  std::shared_ptr<BlockTreeMock> tree_ = std::make_shared<BlockTreeMock>();
  std::shared_ptr<TaggedTransactionQueueMock> tx_queue_ =
      std::make_shared<TaggedTransactionQueueMock>();
  std::shared_ptr<HasherMock> hasher_ = std::make_shared<HasherMock>();
  std::shared_ptr<VRFProviderMock> vrf_provider_ =
      std::make_shared<VRFProviderMock>();
  std::shared_ptr<CSPRNG> generator_ = std::make_shared<BoostRandomGenerator>();
  std::shared_ptr<SR25519ProviderMock> sr25519_provider_ =
      std::make_shared<SR25519ProviderMock>();

  BabeBlockValidator validator_{
      tree_, tx_queue_, hasher_, vrf_provider_, sr25519_provider_};

  // fields for block
  Hash256 parent_hash_ =
      Hash256::fromString("c30ojfn4983u4093jv3894j3f034ojs3").value();

  BabeSlotNumber slot_number_ = 2;
  VRFPreOutput vrf_value_ = {1, 2, 3, 4, 5};
  VRFProof vrf_proof_{};
  AuthorityIndex authority_index_ = {1};
  BabeBlockHeader babe_header_{
      slot_number_, {vrf_value_, vrf_proof_}, authority_index_};
  Buffer encoded_babe_header_{scale::encode(babe_header_).value()};

  BlockHeader block_header_{
      .parent_hash = parent_hash_,
      .digest = {PreRuntime{{kEngineId, encoded_babe_header_}}}};
  Extrinsic ext_{Buffer{0x11, 0x22}};
  BlockBody block_body_{ext_};
  Block valid_block_{block_header_, block_body_};

  Epoch babe_epoch_{.threshold = 3820948573,
                    .randomness = uint256_t_to_bytes(475995757021)};
};

/**
 * @given block validator
 * @when validating a valid block
 * @then success
 */
TEST_F(BlockValidatorTest, Success) {
  // verifySignature
  // get an encoded pre-seal part of the block's header
  auto block_copy = valid_block_;
  block_copy.header.digest.pop_back();
  auto encoded_block_copy = scale::encode(block_copy.header).value();
  Hash256 encoded_block_copy_hash{};  // not a real hash, but don't want to
                                      // actually take it
  std::copy(encoded_block_copy.begin(),
            encoded_block_copy.begin() + Hash256::size(),
            encoded_block_copy_hash.begin());

  auto [seal, pubkey] = sealBlock(valid_block_, encoded_block_copy_hash);

  EXPECT_CALL(*hasher_, blake2b_256(_))
      .WillOnce(Return(encoded_block_copy_hash));

  babe_epoch_.authorities.emplace_back();
  babe_epoch_.authorities.emplace_back(Authority{{pubkey}, 42});

  EXPECT_CALL(*sr25519_provider_, verify(_, _, pubkey))
      .WillOnce(Return(outcome::result<bool>(true)));
  // verifyVRF
  auto randomness_with_slot =
      Buffer{}.put(babe_epoch_.randomness).put(uint64_t_to_bytes(slot_number_));
  EXPECT_CALL(*vrf_provider_, verify(randomness_with_slot, _, pubkey, _))
      .WillOnce(Return(VRFVerifyOutput{.is_valid = true, .is_less = true}));

  primitives::BlockInfo deepest_leaf{1u, createHash256({1u})};
  EXPECT_CALL(*tree_, deepestLeaf()).WillOnce(Return(deepest_leaf));

  // verifyTransactions
  EXPECT_CALL(*tx_queue_, validate_transaction(deepest_leaf.block_number, ext_))
      .WillOnce(Return(Valid{}));

  auto validate_res = validator_.validate(valid_block_, babe_epoch_);
  ASSERT_TRUE(validate_res) << validate_res.error().message();
}

/**
 * @given block validator
 * @when validating block, which has less than two digests
 * @then validation fails
 */
TEST_F(BlockValidatorTest, LessDigestsThanNeeded) {
  babe_epoch_.authorities.emplace_back();

  // for this test we can just not seal the block - it's the second digest
  EXPECT_OUTCOME_FALSE(err, validator_.validate(valid_block_, babe_epoch_));
  ASSERT_EQ(err, BabeBlockValidator::ValidationError::INVALID_DIGESTS);
}

/**
 * @given block validator
 * @when validating block, which does not have a BabeHeader digest
 * @then validation fails
 */
TEST_F(BlockValidatorTest, NoBabeHeader) {
  auto block_copy = valid_block_;
  block_copy.header.digest.pop_back();
  auto encoded_block_copy = scale::encode(block_copy.header).value();
  Hash256 encoded_block_copy_hash{};  // not a real hash, but don't want to
                                      // actually take it
  std::copy(encoded_block_copy.begin(),
            encoded_block_copy.begin() + Hash256::size(),
            encoded_block_copy_hash.begin());

  // take BabeHeader out before sealing the block
  valid_block_.header.digest.pop_back();

  auto [seal, pubkey] = sealBlock(valid_block_, encoded_block_copy_hash);

  babe_epoch_.authorities.emplace_back();
  babe_epoch_.authorities.emplace_back(Authority{{pubkey}, 42});

  EXPECT_OUTCOME_FALSE(err, validator_.validate(valid_block_, babe_epoch_));
  ASSERT_EQ(err, BabeBlockValidator::ValidationError::INVALID_DIGESTS);
}

/**
 * @given block validator
 * @when validating block, which was produced by authority, which we don't know
 * about
 * @then validation fails
 */
TEST_F(BlockValidatorTest, NoAuthority) {
  // GIVEN
  auto block_copy = valid_block_;
  block_copy.header.digest.pop_back();
  auto encoded_block_copy = scale::encode(block_copy.header).value();
  Hash256 encoded_block_copy_hash{};
  std::copy(encoded_block_copy.begin(),
            encoded_block_copy.begin() + Hash256::size(),
            encoded_block_copy_hash.begin());

  sealBlock(valid_block_, encoded_block_copy_hash);

  EXPECT_CALL(*hasher_, blake2b_256(_))
      .WillOnce(Return(encoded_block_copy_hash));

  // WHEN
  // only one authority even though we want at least two
  babe_epoch_.authorities.emplace_back();

  // THEN
  EXPECT_OUTCOME_FALSE(err, validator_.validate(valid_block_, babe_epoch_));
  ASSERT_EQ(err, BabeBlockValidator::ValidationError::INVALID_SIGNATURE);
}

/**
 * @given block validator
 * @when validating block with an invalid signature in the seal
 * @then validation fails
 */
TEST_F(BlockValidatorTest, SignatureVerificationFail) {
  // GIVEN
  auto block_copy = valid_block_;
  block_copy.header.digest.pop_back();
  auto encoded_block_copy = scale::encode(block_copy.header).value();
  Hash256 encoded_block_copy_hash{};
  std::copy(encoded_block_copy.begin(),
            encoded_block_copy.begin() + Hash256::size(),
            encoded_block_copy_hash.begin());

  auto [seal, pubkey] = sealBlock(valid_block_, encoded_block_copy_hash);

  EXPECT_CALL(*hasher_, blake2b_256(_))
      .WillOnce(Return(encoded_block_copy_hash));

  EXPECT_CALL(*sr25519_provider_, verify(_, _, pubkey))
      .WillOnce(Return(outcome::result<bool>(false)));

  babe_epoch_.authorities.emplace_back();
  babe_epoch_.authorities.emplace_back(Authority{{pubkey}, 42});

  // WHEN
  // mutate seal of the block to make signature invalid
  boost::get<kagome::primitives::Seal>(valid_block_.header.digest[1])
      .data[10]++;

  // THEN
  EXPECT_OUTCOME_FALSE(err, validator_.validate(valid_block_, babe_epoch_));
  ASSERT_EQ(err, BabeBlockValidator::ValidationError::INVALID_SIGNATURE);
}

/**
 * @given block validator
 * @when validating block with an invalid VRF proof
 * @then validation fails
 */
TEST_F(BlockValidatorTest, VRFFail) {
  // GIVEN
  auto block_copy = valid_block_;
  block_copy.header.digest.pop_back();
  auto encoded_block_copy = scale::encode(block_copy.header).value();
  Hash256 encoded_block_copy_hash{};
  std::copy(encoded_block_copy.begin(),
            encoded_block_copy.begin() + Hash256::size(),
            encoded_block_copy_hash.begin());

  auto [seal, pubkey] = sealBlock(valid_block_, encoded_block_copy_hash);

  EXPECT_CALL(*hasher_, blake2b_256(_))
      .WillOnce(Return(encoded_block_copy_hash));

  EXPECT_CALL(*sr25519_provider_, verify(_, _, pubkey))
      .WillOnce(Return(outcome::result<bool>(true)));

  babe_epoch_.authorities.emplace_back();
  babe_epoch_.authorities.emplace_back(Authority{{pubkey}, 42});

  // WHEN
  auto randomness_with_slot =
      Buffer{}.put(babe_epoch_.randomness).put(uint64_t_to_bytes(slot_number_));
  EXPECT_CALL(*vrf_provider_, verify(randomness_with_slot, _, pubkey, _))
      .WillOnce(Return(VRFVerifyOutput{.is_valid = false, .is_less = true}));

  // THEN
  EXPECT_OUTCOME_FALSE(err, validator_.validate(valid_block_, babe_epoch_));
  ASSERT_EQ(err, BabeBlockValidator::ValidationError::INVALID_VRF);
}

/**
 * @given block validator
 * @when validating block, which was produced by a non-slot-leader
 * @then validation fails
 */
TEST_F(BlockValidatorTest, ThresholdGreater) {
  // GIVEN
  auto block_copy = valid_block_;
  block_copy.header.digest.pop_back();
  auto encoded_block_copy = scale::encode(block_copy.header).value();
  Hash256 encoded_block_copy_hash{};
  std::copy(encoded_block_copy.begin(),
            encoded_block_copy.begin() + Hash256::size(),
            encoded_block_copy_hash.begin());

  auto [seal, pubkey] = sealBlock(valid_block_, encoded_block_copy_hash);

  EXPECT_CALL(*hasher_, blake2b_256(_))
      .WillOnce(Return(encoded_block_copy_hash));

  EXPECT_CALL(*sr25519_provider_, verify(_, _, pubkey))
      .WillOnce(Return(outcome::result<bool>(true)));

  babe_epoch_.authorities.emplace_back();
  babe_epoch_.authorities.emplace_back(Authority{{pubkey}, 42});

  // WHEN
  babe_epoch_.threshold = 0;

  auto randomness_with_slot =
      Buffer{}.put(babe_epoch_.randomness).put(uint64_t_to_bytes(slot_number_));
  EXPECT_CALL(*vrf_provider_, verify(randomness_with_slot, _, pubkey, _))
      .WillOnce(Return(VRFVerifyOutput{.is_valid = true, .is_less = false}));

  // THEN
  EXPECT_OUTCOME_FALSE(err, validator_.validate(valid_block_, babe_epoch_));
  ASSERT_EQ(err, BabeBlockValidator::ValidationError::INVALID_VRF);
}

/**
 * @given block validator
 * @when validating block, produced by a peer, which has already produced
 * another block in that slot
 * @then validation fails
 */
TEST_F(BlockValidatorTest, TwoBlocksByOnePeer) {
  // GIVEN
  auto block_copy = valid_block_;
  block_copy.header.digest.pop_back();
  auto encoded_block_copy = scale::encode(block_copy.header).value();
  Hash256 encoded_block_copy_hash{};
  std::copy(encoded_block_copy.begin(),
            encoded_block_copy.begin() + Hash256::size(),
            encoded_block_copy_hash.begin());

  auto [seal, pubkey] = sealBlock(valid_block_, encoded_block_copy_hash);

  EXPECT_CALL(*hasher_, blake2b_256(_))
      .Times(2)
      .WillRepeatedly(Return(encoded_block_copy_hash));

  EXPECT_CALL(*sr25519_provider_, verify(_, _, pubkey))
      .Times(2)
      .WillRepeatedly(Return(outcome::result<bool>(true)));

  babe_epoch_.authorities.emplace_back();
  babe_epoch_.authorities.emplace_back(Authority{{pubkey}, 42});

  auto randomness_with_slot =
      Buffer{}.put(babe_epoch_.randomness).put(uint64_t_to_bytes(slot_number_));
  EXPECT_CALL(*vrf_provider_, verify(randomness_with_slot, _, pubkey, _))
      .Times(2)
      .WillRepeatedly(
          Return(VRFVerifyOutput{.is_valid = true, .is_less = true}));

  primitives::BlockInfo deepest_leaf{1u, createHash256({1u})};

  EXPECT_CALL(*tree_, deepestLeaf()).WillOnce(Return(deepest_leaf));

  EXPECT_CALL(*tx_queue_, validate_transaction(deepest_leaf.block_number, ext_))
      .WillOnce(Return(Valid{}));

  // WHEN
  auto validate_res = validator_.validate(valid_block_, babe_epoch_);
  ASSERT_TRUE(validate_res) << validate_res.error().message();

  // THEN
  EXPECT_OUTCOME_FALSE(err, validator_.validate(valid_block_, babe_epoch_));
  ASSERT_EQ(err, BabeBlockValidator::ValidationError::TWO_BLOCKS_IN_SLOT);
}

/**
 * @given block validator
 * @when validating block, which contains an invalid extrinsic
 * @then validation fails
 */
TEST_F(BlockValidatorTest, InvalidExtrinsic) {
  // GIVEN
  auto block_copy = valid_block_;
  block_copy.header.digest.pop_back();
  auto encoded_block_copy = scale::encode(block_copy.header).value();
  Hash256 encoded_block_copy_hash{};
  std::copy(encoded_block_copy.begin(),
            encoded_block_copy.begin() + Hash256::size(),
            encoded_block_copy_hash.begin());

  auto [seal, pubkey] = sealBlock(valid_block_, encoded_block_copy_hash);

  EXPECT_CALL(*hasher_, blake2b_256(_))
      .WillOnce(Return(encoded_block_copy_hash));

  EXPECT_CALL(*sr25519_provider_, verify(_, _, pubkey))
      .WillOnce(Return(outcome::result<bool>(true)));

  babe_epoch_.authorities.emplace_back();
  babe_epoch_.authorities.emplace_back(Authority{{pubkey}, 42});

  auto randomness_with_slot =
      Buffer{}.put(babe_epoch_.randomness).put(uint64_t_to_bytes(slot_number_));
  EXPECT_CALL(*vrf_provider_, verify(randomness_with_slot, _, pubkey, _))
      .WillOnce(Return(VRFVerifyOutput{.is_valid = true, .is_less = true}));

  primitives::BlockInfo deepest_leaf{1u, createHash256({1u})};
  EXPECT_CALL(*tree_, deepestLeaf()).WillOnce(Return(deepest_leaf));

  // WHEN
  EXPECT_CALL(*tx_queue_, validate_transaction(deepest_leaf.block_number, ext_))
      .WillOnce(Return(Invalid{}));

  // THEN
  EXPECT_OUTCOME_FALSE(err, validator_.validate(valid_block_, babe_epoch_));
  ASSERT_EQ(err, BabeBlockValidator::ValidationError::INVALID_TRANSACTIONS);
}
