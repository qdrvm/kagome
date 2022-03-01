/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/mp_utils.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/validation/babe_block_validator.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/crypto/sr25519_provider_mock.hpp"
#include "mock/core/crypto/vrf_provider_mock.hpp"
#include "mock/core/runtime/tagged_transaction_queue_mock.hpp"
#include "mock/libp2p/crypto/random_generator_mock.hpp"
#include "scale/scale.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"
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
using kagome::primitives::InvalidTransaction;
using kagome::primitives::PreRuntime;
using kagome::primitives::TransactionSource;
using kagome::primitives::TransactionValidity;
using kagome::primitives::ValidTransaction;

using testing::_;
using testing::Return;
using testing::ReturnRef;

using testutil::createHash256;

namespace sr25519_constants = kagome::crypto::constants::sr25519;

class BlockValidatorTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  const ConsensusEngineId kEngineId =
      primitives::ConsensusEngineId::fromString("BABE").value();
  /**
   * Add a signature to the block
   * @param block to seal
   * @param block_hash - hash of the block to be sealed
   * @return seal, which was produced, @and public key, which was generated
   */
  std::pair<Seal, Sr25519PublicKey> sealBlock(Block &block,
                                              Hash256 block_hash) const {
    // generate a new keypair
    Sr25519PublicKey public_key{};
    public_key.fill(8);

    Sr25519Signature sr25519_signature{};
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
  std::shared_ptr<Sr25519ProviderMock> sr25519_provider_ =
      std::make_shared<Sr25519ProviderMock>();

  BabeBlockValidator validator_{
      tree_,
      tx_queue_,
      hasher_,
      vrf_provider_,
      sr25519_provider_,
      std::make_shared<primitives::BabeConfiguration>()};

  // fields for block
  Hash256 parent_hash_ =
      Hash256::fromString("c30ojfn4983u4093jv3894j3f034ojs3").value();

  BabeSlotNumber slot_number_ = 2;
  VRFPreOutput vrf_value_ = {1, 2, 3, 4, 5};
  VRFProof vrf_proof_{};
  AuthorityIndex authority_index_ = {1};
  BabeBlockHeader babe_header_{SlotType::Primary,
                               slot_number_,
                               {vrf_value_, vrf_proof_},
                               authority_index_};
  Buffer encoded_babe_header_{scale::encode(babe_header_).value()};

  BlockHeader block_header_{
      .parent_hash = parent_hash_,
      .digest = {PreRuntime{{kEngineId, encoded_babe_header_}}}};
  Extrinsic ext_{Buffer{0x11, 0x22}};
  BlockBody block_body_{ext_};
  Block valid_block_{block_header_, block_body_};

  Threshold threshold_ = 3820948573;
  primitives::AuthorityList authorities_;
  Randomness randomness_{uint256_to_le_bytes(475995757021)};
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

  auto authority = Authority{{pubkey}, 42};
  authorities_.emplace_back();
  authorities_.emplace_back(authority);

  EXPECT_CALL(*sr25519_provider_, verify(_, _, pubkey))
      .WillOnce(Return(outcome::result<bool>(true)));
  // verifyVRF
  EXPECT_CALL(*vrf_provider_, verifyTranscript(_, _, pubkey, _))
      .WillOnce(Return(VRFVerifyOutput{.is_valid = true, .is_less = true}));

  auto validate_res = validator_.validateHeader(
      valid_block_.header, 0ull, authority.id, threshold_, randomness_);
  ASSERT_TRUE(validate_res) << validate_res.error().message();
}

/**
 * @given block validator
 * @when validating block, which has less than two digests
 * @then validation fails
 */
TEST_F(BlockValidatorTest, LessDigestsThanNeeded) {
  auto authority = Authority{{}, 42};
  authorities_.emplace_back(authority);

  // for this test we can just not seal the block - it's the second digest
  EXPECT_OUTCOME_FALSE(
      err,
      validator_.validateHeader(
          valid_block_.header, 0ull, authority.id, threshold_, randomness_));
  ASSERT_EQ(err, kagome::consensus::DigestError::REQUIRED_DIGESTS_NOT_FOUND);
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

  auto authority = Authority{{pubkey}, 42};
  authorities_.emplace_back();
  authorities_.emplace_back(authority);

  EXPECT_OUTCOME_FALSE(
      err,
      validator_.validateHeader(
          valid_block_.header, 0ull, authority.id, threshold_, randomness_));
  ASSERT_EQ(err, consensus::DigestError::REQUIRED_DIGESTS_NOT_FOUND);
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

  const auto &[seal, public_key] =
      sealBlock(valid_block_, encoded_block_copy_hash);

  EXPECT_CALL(*hasher_, blake2b_256(_))
      .WillOnce(Return(encoded_block_copy_hash));

  // WHEN
  // only one authority even though we want at least two
  auto authority = Authority{{}, 42};

  EXPECT_CALL(
      *sr25519_provider_,
      verify(
          seal.signature, _, kagome::crypto::Sr25519PublicKey{authority.id.id}))
      .WillOnce(Return(false));

  // THEN
  EXPECT_OUTCOME_ERROR(
      err,
      validator_.validateHeader(
          valid_block_.header, 0ull, authority.id, threshold_, randomness_),
      BabeBlockValidator::ValidationError::INVALID_SIGNATURE);
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

  authorities_.emplace_back();
  auto authority = Authority{{pubkey}, 42};
  authorities_.emplace_back(authority);

  // WHEN
  // mutate seal of the block to make signature invalid
  boost::get<kagome::primitives::Seal>(valid_block_.header.digest[1])
      .data[10]++;

  // THEN
  EXPECT_OUTCOME_FALSE(
      err,
      validator_.validateHeader(
          valid_block_.header, 0ull, authority.id, threshold_, randomness_));
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

  authorities_.emplace_back();
  auto authority = Authority{{pubkey}, 42};
  authorities_.emplace_back(authority);

  // WHEN
  EXPECT_CALL(*vrf_provider_, verifyTranscript(_, _, pubkey, _))
      .WillOnce(Return(VRFVerifyOutput{.is_valid = false, .is_less = true}));

  // THEN
  EXPECT_OUTCOME_FALSE(
      err,
      validator_.validateHeader(
          valid_block_.header, 0ull, authority.id, threshold_, randomness_));
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

  authorities_.emplace_back();
  auto authority = Authority{{pubkey}, 42};
  authorities_.emplace_back(authority);

  // WHEN
  threshold_ = 0;

  EXPECT_CALL(*vrf_provider_, verifyTranscript(_, _, pubkey, _))
      .WillOnce(Return(VRFVerifyOutput{.is_valid = true, .is_less = false}));

  // THEN
  EXPECT_OUTCOME_FALSE(
      err,
      validator_.validateHeader(
          valid_block_.header, 0ull, authority.id, threshold_, randomness_));
  ASSERT_EQ(err, BabeBlockValidator::ValidationError::INVALID_VRF);
}
