/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/vrf/vrf_provider_impl.hpp"

#include <gtest/gtest.h>
#include "common/int_serialization.hpp"
#include "crypto/random_generator/boost_generator.hpp"

using kagome::common::Buffer;
using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::Sr25519Keypair;
using kagome::crypto::VRFPreOutput;
using kagome::crypto::VRFProviderImpl;
using kagome::crypto::VRFThreshold;
using kagome::primitives::Transcript;

class VRFProviderTest : public testing::Test {
 public:
  void SetUp() override {
    vrf_provider_ = std::make_shared<VRFProviderImpl>(
        std::make_shared<BoostRandomGenerator>());
    keypair1_ = vrf_provider_->generateKeypair();
    keypair2_ = vrf_provider_->generateKeypair();
    msg_ = Buffer{1, 2, 3};
  }

 protected:
  std::shared_ptr<VRFProviderImpl> vrf_provider_;
  Sr25519Keypair keypair1_;
  Sr25519Keypair keypair2_;
  Buffer msg_;
  const Buffer reference_data{
      156, 127, 91,  234, 138, 145, 60,  180, 10,  209, 13,  13,  101, 100, 39,
      7,   179, 97,  106, 47,  48,  101, 34,  246, 115, 59,  228, 32,  179, 45,
      247, 57,  200, 13,  27,  66,  9,   122, 201, 124, 247, 39,  21,  71,  115,
      230, 19,  148, 34,  78,  72,  254, 182, 45,  51,  18,  147, 204, 146, 218,
      180, 71,  217, 132, 147, 211, 110, 225, 195, 71,  203, 148, 171, 45,  237,
      178, 105, 149, 194, 127, 124, 132, 19,  116, 209, 255, 88,  152, 134, 60,
      131, 11,  10,  111, 28,  83,  83,  168, 68,  4,   86,  106, 109, 54,  58,
      191, 155, 27,  146, 183, 233, 7,   163, 86,  38,  172, 160, 188, 126, 136,
      101, 111, 203, 69,  174, 4,   188, 52,  202, 190, 174, 190, 121, 217, 23,
      80,  192, 232, 191, 19,  185, 102, 80,  77,  19,  67,  89,  114, 101, 221,
      136, 101, 173, 249, 20,  9,   204, 155, 32,  213, 244, 116, 68,  4,   31,
      151, 182, 153, 221, 251, 222, 233, 30,  168, 123, 208, 155, 248, 176, 45,
      167, 90,  150, 233, 71,  240, 127, 91,  101, 187, 78,  110, 254, 250, 161,
      106, 191, 217, 251, 246, 144, 111, 2};

  inline void prepare_transcript(Transcript &t) {
    t.initialize({'I', 'D', 'D', 'Q', 'D'});
  }
};

/**
 * @given vrf provider @and very large threshold value @and some message
 * @when we derive vrf value and proof from signing the message
 * @then output value is less than threshold @and proof verifies that value was
 * generated using vrf
 */
TEST_F(VRFProviderTest, SignAndVerifySuccess) {
  // given
  VRFThreshold threshold{std::numeric_limits<VRFThreshold>::max() - 1};
  // when
  Buffer src(reference_data);
  auto out_opt = vrf_provider_->sign(src, keypair1_, threshold);
  ASSERT_TRUE(out_opt);
  auto out = out_opt.value();

  // then
  Buffer dst(reference_data);
  auto verify_res =
      vrf_provider_->verify(dst, out, keypair1_.public_key, threshold);
  ASSERT_TRUE(verify_res.is_valid);
  ASSERT_TRUE(verify_res.is_less);
}

/**
 * @given vrf provider @and very large threshold value @and some transcript
 * @when we derive vrf value and proof from signing the message
 * @then output value is less than threshold @and proof verifies that value was
 * generated using vrf
 */
TEST_F(VRFProviderTest, SignAndVerifyTranscriptSuccess) {
  // given
  VRFThreshold threshold{std::numeric_limits<VRFThreshold>::max() - 1};
  Transcript t_src;
  prepare_transcript(t_src);

  // when
  auto out_opt = vrf_provider_->signTranscript(t_src, keypair1_, threshold);
  ASSERT_TRUE(out_opt);
  auto out = out_opt.value();

  // then
  Transcript t_dst;
  prepare_transcript(t_dst);
  auto verify_res = vrf_provider_->verifyTranscript(
      t_dst, out, keypair1_.public_key, threshold);
  ASSERT_TRUE(verify_res.is_valid);
  ASSERT_TRUE(verify_res.is_less);
}

/**
 * @given vrf provider @and very small threshold value @and some transcript
 * @when we try to derive vrf output from signing the message
 * @then output is not created as value is bigger than threshold
 */
TEST_F(VRFProviderTest, TranscriptSignFailed) {
  // given
  VRFThreshold threshold{std::numeric_limits<VRFPreOutput>::min()};
  Transcript t_src;
  prepare_transcript(t_src);

  // when
  auto out_opt = vrf_provider_->signTranscript(t_src, keypair1_, threshold);

  // then
  ASSERT_FALSE(out_opt.has_value());
}

/**
 * @given vrf provider @and very large threshold value @and some transcript
 * @when we derive vrf value and proof from signing the message @and try to
 * verify proof by wrong public key
 * @then output value is less than threshold @and proof is not verified
 */
TEST_F(VRFProviderTest, TranscriptVerifyFailed) {
  // given
  VRFThreshold threshold{std::numeric_limits<VRFThreshold>::max() - 1};
  Transcript t_src;
  prepare_transcript(t_src);

  // when
  auto out_opt = vrf_provider_->signTranscript(t_src, keypair1_, threshold);
  ASSERT_TRUE(out_opt);
  auto out = out_opt.value();

  // then
  Transcript t_dst;
  prepare_transcript(t_dst);
  ASSERT_FALSE(
      vrf_provider_
          ->verifyTranscript(t_dst, out, keypair2_.public_key, threshold)
          .is_valid);
}

/**
 * @given vrf provider @and very large threshold value @and some message
 * @when we derive vrf value and proof from signing the message @and try to
 * verify proof by wrong public key
 * @then output value is less than threshold @and proof is not verified
 */
TEST_F(VRFProviderTest, VerifyFailed) {
  // given
  VRFThreshold threshold{std::numeric_limits<VRFThreshold>::max() - 1};

  // when
  Buffer src(reference_data);
  auto out_opt = vrf_provider_->sign(src, keypair1_, threshold);
  ASSERT_TRUE(out_opt);
  auto out = out_opt.value();

  // then
  Buffer dst(reference_data);
  ASSERT_FALSE(vrf_provider_->verify(dst, out, keypair2_.public_key, threshold)
                   .is_valid);
}

/**
 * @given vrf provider @and very small threshold value @and some message
 * @when we try to derive vrf output from signing the message
 * @then output is not created as value is bigger than threshold
 */
TEST_F(VRFProviderTest, SignFailed) {
  // given
  VRFThreshold threshold{std::numeric_limits<VRFPreOutput>::min()};

  // when
  Buffer src(reference_data);
  auto out_opt = vrf_provider_->sign(src, keypair1_, threshold);

  // then
  ASSERT_FALSE(out_opt.has_value());
}
