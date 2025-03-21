/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/vrf/vrf_provider_impl.hpp"

#include <gtest/gtest.h>

#include "crypto/random_generator/boost_generator.hpp"
#include "parachain/approval/transcript_utils.hpp"
#include "parachain/types.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::common::Buffer;
using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::Sr25519Keypair;
using kagome::crypto::VRFPreOutput;
using kagome::crypto::VRFProviderImpl;
using kagome::crypto::VRFThreshold;
using kagome::primitives::Transcript;

class VRFProviderTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

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
  VRFThreshold threshold{std::numeric_limits<VRFThreshold>::min()};
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
  VRFThreshold threshold{std::numeric_limits<VRFThreshold>::min()};

  // when
  Buffer src(reference_data);
  auto out_opt = vrf_provider_->sign(src, keypair1_, threshold);

  // then
  ASSERT_FALSE(out_opt.has_value());
}

/**
 * @given data for vrf_verify_extra that are correctly verified in Polkadot-SDK
 * @when same data are verified using sr25519_vrf_verify_extra
 * @then sr25519_vrf_verify_extra returns SR25519_SIGNATURE_RESULT_OK
 *
 * Possible implementation of the same test in Rust:
```rust
#[test]
fn test_relay_vrf_modulo_transcript_v1() {
        // create public key from array of bytes
        let public_res = schnorrkel::PublicKey::from_bytes(&[
                212, 53, 147, 199, 21, 253, 211, 28, 97, 20, 26, 189, 4, 169,
159, 214, 130, 44, 133, 88, 133, 76, 205, 227, 154, 86, 132, 231, 165, 109, 162,
125,
        ]);
        assert!(public_res.is_ok());
        let public = public_res.unwrap();

        let relay_vrf_story = RelayVRFStory([
                40, 81, 9, 6, 181, 210, 226, 0, 178, 152, 8, 24, 87, 67, 12,
150, 126, 158, 110, 60, 236, 152, 130, 39, 194, 76, 50, 108, 182, 66, 55, 244,
        ]);

        let vrf_pre_output_bytes = [
                186, 162, 249, 255, 191, 230, 212, 25, 49, 79, 148, 184, 71, 24,
252, 53, 205, 131, 9, 108, 40, 175, 127, 118, 43, 152, 121, 176, 174, 52, 199,
95,
        ];
        // Convert raw bytes to schnorrkel::vrf::VRFPreOut
        let vrf_pre_out =
schnorrkel::vrf::VRFPreOut::from_bytes(&vrf_pre_output_bytes) .expect("Valid
VRFPreOut bytes");

        // Wrap in VrfPreOutput
        let vrf_pre_output = VrfPreOutput(vrf_pre_out);

        let vrf_proof_bytes = [
                51, 16, 135, 168, 206, 210, 39, 130, 221, 215, 8, 129, 160, 131,
232, 46, 114, 84, 184, 28, 51, 109, 137, 147, 168, 201, 144, 169, 193, 81, 151,
10, 8, 244, 195, 225, 254, 134, 215, 234, 206, 179, 100, 242, 36, 7, 20, 14, 26,
156, 29, 223, 121, 159, 243, 213, 44, 143, 113, 27, 168, 249, 2, 8,
        ];
        let vrf_proof =
                schnorrkel::vrf::VRFProof::from_bytes(&vrf_proof_bytes).expect("Valid
VRFProof bytes");

        let sample: u32 = 0;

        let transcript = relay_vrf_modulo_transcript_v1(relay_vrf_story,
sample); let core_index: u32 = 6; let assigned_transcript =
assigned_core_transcript(CoreIndex(core_index));

        let res =
                public.vrf_verify_extra(transcript, &vrf_pre_output.0,
&vrf_proof, assigned_transcript); assert!(res.is_ok());
}
```
 */
TEST_F(VRFProviderTest, VrfVerifyExtra) {
  auto pubkey_init_res = kagome::crypto::Sr25519PublicKey::fromSpan(
      Buffer{212, 53,  147, 199, 21,  253, 211, 28,  97,  20,  26,
             189, 4,   169, 159, 214, 130, 44,  133, 88,  133, 76,
             205, 227, 154, 86,  132, 231, 165, 109, 162, 125});
  ASSERT_TRUE(pubkey_init_res.has_value());
  kagome::crypto::Sr25519PublicKey public_key = pubkey_init_res.value();

  RelayVRFStory relay_vrf_story{.data = {
                                    40,  81,  9,   6,   181, 210, 226, 0,
                                    178, 152, 8,   24,  87,  67,  12,  150,
                                    126, 158, 110, 60,  236, 152, 130, 39,
                                    194, 76,  50,  108, 182, 66,  55,  244,
                                }};
  VRFPreOutput vrf_pre_output{186, 162, 249, 255, 191, 230, 212, 25,
                              49,  79,  148, 184, 71,  24,  252, 53,
                              205, 131, 9,   108, 40,  175, 127, 118,
                              43,  152, 121, 176, 174, 52,  199, 95};
  kagome::crypto::VRFProof vrf_proof{
      51,  16,  135, 168, 206, 210, 39,  130, 221, 215, 8,   129, 160,
      131, 232, 46,  114, 84,  184, 28,  51,  109, 137, 147, 168, 201,
      144, 169, 193, 81,  151, 10,  8,   244, 195, 225, 254, 134, 215,
      234, 206, 179, 100, 242, 36,  7,   20,  14,  26,  156, 29,  223,
      121, 159, 243, 213, 44,  143, 113, 27,  168, 249, 2,   8};
  uint32_t sample = 0;

  auto modulo_transcript =
      relay_vrf_modulo_transcript_v1(relay_vrf_story, sample);

  uint32_t first_claimed_core_index = 6;
  auto assigned_transcript = assigned_core_transcript(first_claimed_core_index);

  auto res = sr25519_vrf_verify_extra(
      public_key.data(),
      vrf_pre_output.data(),
      vrf_proof.data(),
      reinterpret_cast<const Strobe128 *>(modulo_transcript.data().data()),
      reinterpret_cast<const Strobe128 *>(assigned_transcript.data().data()));
  ASSERT_EQ(res.result, SR25519_SIGNATURE_RESULT_OK);
}

/**
 * @given data for vrf_verify_extra that are correctly verified in Polkadot-SDK
 * @when same data are verified using sr25519_vrf_verify_extra with garbage key
 * @then sr25519_vrf_verify_extra returns SR25519_SIGNATURE_RESULT_POINT_DECOMPRESSION_ERROR
 */
TEST_F(VRFProviderTest, VrfVerifyExtraWithGarbageKey) {
  // Create a garbage public key filled with 0xFF bytes
  Buffer garbage_key_bytes(32, 0xFF);
  auto pubkey_init_res = kagome::crypto::Sr25519PublicKey::fromSpan(garbage_key_bytes);

  // The key initialization might fail, but we need a key object to pass to the verify function
  kagome::crypto::Sr25519PublicKey garbage_key;
  if (pubkey_init_res.has_value()) {
    garbage_key = pubkey_init_res.value();
  } else {
    // If fromSpan fails, create a default key - the verification should still fail
    garbage_key = kagome::crypto::Sr25519PublicKey{};
  }

  RelayVRFStory relay_vrf_story{.data = {
                                    40,  81,  9,   6,   181, 210, 226, 0,
                                    178, 152, 8,   24,  87,  67,  12,  150,
                                    126, 158, 110, 60,  236, 152, 130, 39,
                                    194, 76,  50,  108, 182, 66,  55,  244,
                                }};
  VRFPreOutput vrf_pre_output{186, 162, 249, 255, 191, 230, 212, 25,
                              49,  79,  148, 184, 71,  24,  252, 53,
                              205, 131, 9,   108, 40,  175, 127, 118,
                              43,  152, 121, 176, 174, 52,  199, 95};
  kagome::crypto::VRFProof vrf_proof{
      51,  16,  135, 168, 206, 210, 39,  130, 221, 215, 8,   129, 160,
      131, 232, 46,  114, 84,  184, 28,  51,  109, 137, 147, 168, 201,
      144, 169, 193, 81,  151, 10,  8,   244, 195, 225, 254, 134, 215,
      234, 206, 179, 100, 242, 36,  7,   20,  14,  26,  156, 29,  223,
      121, 159, 243, 213, 44,  143, 113, 27,  168, 249, 2,   8};
  uint32_t sample = 0;

  auto modulo_transcript =
      relay_vrf_modulo_transcript_v1(relay_vrf_story, sample);

  uint32_t first_claimed_core_index = 6;
  auto assigned_transcript = assigned_core_transcript(first_claimed_core_index);

  auto res = sr25519_vrf_verify_extra(
      garbage_key.data(),
      vrf_pre_output.data(),
      vrf_proof.data(),
      reinterpret_cast<const Strobe128 *>(modulo_transcript.data().data()),
      reinterpret_cast<const Strobe128 *>(assigned_transcript.data().data()));

  // With garbage key, we expect Signature error
  ASSERT_EQ(res.result, SR25519_SIGNATURE_RESULT_POINT_DECOMPRESSION_ERROR);
}