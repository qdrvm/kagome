/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "consensus/babe/impl/babe_block_validator_impl.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/types/seal.hpp"
#include "mock/core/consensus/babe/babe_config_repository_mock.hpp"
#include "mock/core/consensus/timeline/slots_util_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/crypto/sr25519_provider_mock.hpp"
#include "mock/core/crypto/vrf_provider_mock.hpp"
#include "testutil/lazy.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/sr25519_utils.hpp"

using kagome::common::Buffer;
using kagome::common::uint256_to_le_bytes;
using kagome::consensus::SlotNumber;
using kagome::consensus::SlotsUtil;
using kagome::consensus::SlotsUtilMock;
using kagome::consensus::Threshold;
using kagome::consensus::babe::Authorities;
using kagome::consensus::babe::Authority;
using kagome::consensus::babe::AuthorityId;
using kagome::consensus::babe::AuthorityIndex;
using kagome::consensus::babe::BabeBlockHeader;
using kagome::consensus::babe::BabeBlockValidatorImpl;
using kagome::consensus::babe::BabeConfigRepositoryMock;
using kagome::consensus::babe::BabeConfiguration;
using kagome::consensus::babe::DigestError;
using kagome::consensus::babe::Randomness;
using kagome::consensus::babe::SlotType;
using kagome::crypto::HasherMock;
using kagome::crypto::Sr25519Keypair;
using kagome::crypto::Sr25519ProviderMock;
using kagome::crypto::Sr25519PublicKey;
using kagome::crypto::Sr25519Signature;
using kagome::crypto::VRFPreOutput;
using kagome::crypto::VRFProof;
using kagome::crypto::VRFProviderMock;
using kagome::crypto::VRFVerifyOutput;
using kagome::primitives::Block;
using kagome::primitives::BlockBody;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::ConsensusEngineId;
using kagome::primitives::Extrinsic;
using kagome::primitives::PreRuntime;
using Seal = kagome::consensus::babe::Seal;
using ValidatingError =
    kagome::consensus::babe::BabeBlockValidatorImpl::ValidationError;

using testing::_;
using testing::Invoke;
using testing::Mock;
using testing::Return;

class BabeBlockValidatorTest : public testing::Test {
 public:
  const ConsensusEngineId kEngineId =
      ConsensusEngineId::fromString("BABE").value();

  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    slots_util = std::make_shared<SlotsUtilMock>();
    ON_CALL(*slots_util, slotToEpoch(_, _)).WillByDefault(Return(1));

    config = std::make_shared<BabeConfiguration>();
    config->randomness.fill(0);
    config->leadership_rate = {1, 4};

    config_repo = std::make_shared<BabeConfigRepositoryMock>();
    ON_CALL(*config_repo, config(_, _)).WillByDefault(Invoke([&] {
      config->authorities = authorities;
      return config;
    }));

    hasher = std::make_shared<HasherMock>();

    sr25519_provider = std::make_shared<Sr25519ProviderMock>();
    vrf_provider = std::make_shared<VRFProviderMock>();

    block_validator = std::make_shared<BabeBlockValidatorImpl>(
        testutil::sptr_to_lazy<SlotsUtil>(slots_util),
        config_repo,
        hasher,
        sr25519_provider,
        vrf_provider);
  }

  std::shared_ptr<SlotsUtilMock> slots_util;
  std::shared_ptr<Sr25519Keypair> our_keypair;
  std::shared_ptr<Sr25519Keypair> other_keypair;
  std::shared_ptr<BabeConfiguration> config;
  std::shared_ptr<BabeConfigRepositoryMock> config_repo;
  std::shared_ptr<HasherMock> hasher;
  std::shared_ptr<Sr25519ProviderMock> sr25519_provider;
  std::shared_ptr<VRFProviderMock> vrf_provider;
  std::shared_ptr<BabeBlockValidatorImpl> block_validator;

  // fields for block
  BlockHash parent_hash_ =
      BlockHash::fromString("c30ojfn4983u4093jv3894j3f034ojs3").value();

  SlotNumber slot_number_ = 2;
  VRFPreOutput vrf_value_ = {1, 2, 3, 4, 5};
  VRFProof vrf_proof_{};
  AuthorityIndex authority_index_ = {1};
  BabeBlockHeader babe_header_{SlotType::Primary,
                               authority_index_,
                               slot_number_,
                               {vrf_value_, vrf_proof_}};
  Buffer encoded_babe_header_{scale::encode(babe_header_).value()};

  BlockHeader block_header_{123,           // number
                            parent_hash_,  // parent
                            {},            // state root
                            {},            // extrinsics root
                            {PreRuntime{{kEngineId, encoded_babe_header_}}}};
  Extrinsic ext_{Buffer{0x11, 0x22}};
  BlockBody block_body_{ext_};
  Block valid_block_{block_header_, block_body_};

  Threshold threshold_ = 3820948573;

  BabeConfiguration config_{
      .leadership_rate = {3, 4},
      .authorities = {},
      .randomness = Randomness{uint256_to_le_bytes(475995757021)},
      .allowed_slots = {},
  };

  Authorities authorities;

  /**
   * Add a signature to the block
   * @param block to seal
   * @param block_hash - hash of the block to be sealed
   * @return seal, which was produced, @and public key, which was generated
   */
  std::pair<Seal, Sr25519PublicKey> sealBlock(
      Block &block, const BlockHash &block_hash) const {
    // generate a new keypair
    Sr25519PublicKey public_key{};
    public_key.fill(8);

    Sr25519Signature sr25519_signature{};
    sr25519_signature.fill(0);

    // seal the block
    Seal seal{sr25519_signature};
    Buffer encoded_seal{scale::encode(seal).value()};
    block.header.digest.push_back(
        kagome::primitives::Seal{{kEngineId, encoded_seal}});

    return {seal, public_key};
  }
};

/**
 * @given block validator
 * @when validating a valid block
 * @then success
 */
TEST_F(BabeBlockValidatorTest, Success) {
  // verifySignature
  // get an encoded pre-seal part of the block's header
  auto block_copy = valid_block_;
  block_copy.header.digest.pop_back();
  auto encoded_block_copy = scale::encode(block_copy.header).value();
  BlockHash encoded_block_copy_hash{};  // not a real hash, but don't want to
                                        // actually take it
  std::copy(encoded_block_copy.begin(),
            encoded_block_copy.begin() + BlockHash::size(),
            encoded_block_copy_hash.begin());

  auto [seal, pubkey] = sealBlock(valid_block_, encoded_block_copy_hash);

  EXPECT_CALL(*hasher, blake2b_256(_))
      .WillOnce(Return(encoded_block_copy_hash));

  auto authority = Authority{AuthorityId{pubkey}, 42};
  authorities.emplace_back();
  authorities.emplace_back(authority);

  EXPECT_CALL(*sr25519_provider, verify(_, _, pubkey))
      .WillOnce(Return(outcome::result<bool>(true)));
  // verifyVRF
  EXPECT_CALL(*vrf_provider, verifyTranscript(_, _, pubkey, _))
      .WillOnce(Return(VRFVerifyOutput{.is_valid = true, .is_less = true}));

  auto validate_res = block_validator->validateHeader(valid_block_.header);
  EXPECT_OUTCOME_TRUE_1(validate_res);
}

/**
 * @given block validator
 * @when validating block, which has less than two digests
 * @then validation fails
 */
TEST_F(BabeBlockValidatorTest, LessDigestsThanNeeded) {
  auto authority = Authority{{}, 42};
  authorities.emplace_back(authority);

  // for this test we can just not seal the block - it's the second digest
  EXPECT_OUTCOME_FALSE(err,
                       block_validator->validateHeader(valid_block_.header));
  ASSERT_EQ(err, DigestError::REQUIRED_DIGESTS_NOT_FOUND);
}

/**
 * @given block validator
 * @when validating block, which does not have a BabeHeader digest
 * @then validation fails
 */
TEST_F(BabeBlockValidatorTest, NoBabeHeader) {
  auto block_copy = valid_block_;
  block_copy.header.digest.pop_back();
  auto encoded_block_copy = scale::encode(block_copy.header).value();
  BlockHash encoded_block_copy_hash{};  // not a real hash, but don't want to
                                        // actually take it
  std::copy(encoded_block_copy.begin(),
            encoded_block_copy.begin() + BlockHash::size(),
            encoded_block_copy_hash.begin());

  // take BabeHeader out before sealing the block
  valid_block_.header.digest.pop_back();

  auto [seal, pubkey] = sealBlock(valid_block_, encoded_block_copy_hash);

  authorities.emplace_back();
  authorities.emplace_back(Authority{pubkey, 42});

  EXPECT_OUTCOME_FALSE(err,
                       block_validator->validateHeader(valid_block_.header));
  ASSERT_EQ(err, DigestError::REQUIRED_DIGESTS_NOT_FOUND);
}

/**
 * @given block validator
 * @when validating block with an invalid signature in the seal
 * @then validation fails
 */
TEST_F(BabeBlockValidatorTest, SignatureVerificationFail) {
  // GIVEN
  auto block_copy = valid_block_;
  block_copy.header.digest.pop_back();
  auto encoded_block_copy = scale::encode(block_copy.header).value();
  BlockHash encoded_block_copy_hash{};
  std::copy(encoded_block_copy.begin(),
            encoded_block_copy.begin() + BlockHash::size(),
            encoded_block_copy_hash.begin());

  auto [seal, pubkey] = sealBlock(valid_block_, encoded_block_copy_hash);

  authorities.emplace_back();
  authorities.emplace_back(Authority{pubkey, 42});

  EXPECT_CALL(*hasher, blake2b_256(_))
      .WillOnce(Return(encoded_block_copy_hash));

  EXPECT_CALL(*sr25519_provider, verify(_, _, _)).WillOnce(Return(false));

  // WHEN-THEN
  EXPECT_OUTCOME_FALSE(err,
                       block_validator->validateHeader(valid_block_.header));
  ASSERT_EQ(err, ValidatingError::INVALID_SIGNATURE);
}

/**
 * @given block validator
 * @when validating block with an invalid VRF proof
 * @then validation fails
 */
TEST_F(BabeBlockValidatorTest, VRFFail) {
  // GIVEN
  auto block_copy = valid_block_;
  block_copy.header.digest.pop_back();
  auto encoded_block_copy = scale::encode(block_copy.header).value();
  BlockHash encoded_block_copy_hash{};
  std::copy_n(encoded_block_copy.begin(),
              BlockHash::size(),
              encoded_block_copy_hash.begin());

  auto [seal, pubkey] = sealBlock(valid_block_, encoded_block_copy_hash);

  authorities.emplace_back();
  authorities.emplace_back(Authority{pubkey, 42});

  EXPECT_CALL(*hasher, blake2b_256(_))
      .WillOnce(Return(encoded_block_copy_hash));

  EXPECT_CALL(*sr25519_provider, verify(_, _, pubkey)).WillOnce(Return(true));

  // WHEN
  EXPECT_CALL(*vrf_provider, verifyTranscript(_, _, pubkey, _))
      .WillOnce(Return(VRFVerifyOutput{.is_valid = false, .is_less = true}));

  // THEN
  EXPECT_OUTCOME_FALSE(err,
                       block_validator->validateHeader(valid_block_.header));
  ASSERT_EQ(err, ValidatingError::INVALID_VRF);
}

/**
 * @given block validator
 * @when validating block, which was produced by a non-slot-leader
 * @then validation fails
 */
TEST_F(BabeBlockValidatorTest, ThresholdGreater) {
  // GIVEN
  auto block_copy = valid_block_;
  block_copy.header.digest.pop_back();
  auto encoded_block_copy = scale::encode(block_copy.header).value();
  BlockHash encoded_block_copy_hash{};
  std::copy(encoded_block_copy.begin(),
            encoded_block_copy.begin() + BlockHash::size(),
            encoded_block_copy_hash.begin());

  auto [seal, pubkey] = sealBlock(valid_block_, encoded_block_copy_hash);

  EXPECT_CALL(*hasher, blake2b_256(_))
      .WillOnce(Return(encoded_block_copy_hash));

  EXPECT_CALL(*sr25519_provider, verify(_, _, pubkey))
      .WillOnce(Return(outcome::result<bool>(true)));

  authorities.emplace_back();
  auto authority = Authority{{pubkey}, 42};
  authorities.emplace_back(authority);

  // WHEN
  threshold_ = 0;

  EXPECT_CALL(*vrf_provider, verifyTranscript(_, _, pubkey, _))
      .WillOnce(Return(VRFVerifyOutput{.is_valid = true, .is_less = false}));

  // THEN
  EXPECT_OUTCOME_FALSE(err,
                       block_validator->validateHeader(valid_block_.header));
  ASSERT_EQ(err, ValidatingError::INVALID_VRF);
}
