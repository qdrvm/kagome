/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "crypto/vrf/vrf_provider_impl.hpp"

#include <gtest/gtest.h>
#include <gsl/span>
#include "crypto/random_generator/boost_generator.hpp"
#include "testutil/outcome.hpp"
#include "common/blob.hpp"
#include "primitives/session_key.hpp"
#include "primitives/transcript.hpp"
#include "primitives/babe_configuration.hpp"
#include "crypto/sr25519_types.hpp"
#include "consensus/babe/common.hpp"
#include "common/mp_utils.hpp"

using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::CSPRNG;
using kagome::crypto::Sr25519Provider;
using kagome::crypto::Sr25519ProviderImpl;
using kagome::crypto::Sr25519PublicKey;
using kagome::crypto::Sr25519SecretKey;
using kagome::crypto::Sr25519Seed;
using kagome::crypto::VRFProviderImpl;
using kagome::crypto::VRFProvider;


using kagome::crypto::VRFThreshold;
using kagome::common::Blob;
using kagome::common::Buffer;
using kagome::primitives::BabeSessionKey;
using kagome::primitives::Transcript;
using kagome::primitives::Randomness;
using kagome::primitives::BabeSlotNumber;
using kagome::crypto::VRFOutput;
using kagome::consensus::EpochIndex;

namespace {
  static Transcript &makeTranscript(
      Transcript &transcript_,
      const Randomness &randomness,
      BabeSlotNumber slot_number,
      EpochIndex epoch) {
    transcript_.initialize({'B', 'A', 'B', 'E'});
    transcript_.append_message(
        {'s', 'l', 'o', 't', ' ', 'n', 'u', 'm', 'b', 'e', 'r'}, slot_number);
    transcript_.append_message(
        {'c', 'u', 'r', 'r', 'e', 'n', 't', ' ', 'e', 'p', 'o', 'c', 'h'},
        epoch);
    transcript_.append_message({'c',
                                'h',
                                'a',
                                'i',
                                'n',
                                ' ',
                                'r',
                                'a',
                                'n',
                                'd',
                                'o',
                                'm',
                                'n',
                                'e',
                                's',
                                's'},
                               randomness.internal_array_reference());
    return transcript_;
  }

}


struct Sr25519ProviderTest : public ::testing::Test {
  void SetUp() override {
    random_generator = std::make_shared<BoostRandomGenerator>();
    sr25519_provider = std::make_shared<Sr25519ProviderImpl>(random_generator);
    vrf_provider = std::make_shared<VRFProviderImpl>(random_generator);

    std::string_view m = "i am a message";
    message.clear();
    message.reserve(m.length());
    for (auto c : m) {
      message.push_back(c);
    }

    hex_seed =
        "31102468cbd502d177793fa523685b248f6bd083d67f76671e0b86d7fa20c030";
    hex_sk =
        "e5aff1a7d9694f2c0505f41ca68d51093d4f9f897aaa3ec4116b80393690010bbb5ee"
        "1ea15ca731e60cd92b0765cf00675bb7eeabc04e531629988cd90e53ad6";
    hex_vk = "6221d74b4c2168d0f73f97589900d2c6bdcdf3a8d54c3c92adc9e7650fbff251";
  }

  std::string_view hex_seed;
  std::string_view hex_sk;
  std::string_view hex_vk;

  gsl::span<uint8_t> message_span;
  std::vector<uint8_t> message;
  std::shared_ptr<CSPRNG> random_generator;
  std::shared_ptr<Sr25519Provider> sr25519_provider;
  std::shared_ptr<VRFProvider> vrf_provider;
};

struct ThinTranscript {
  uint8_t data[200];
  uint8_t pos;
  uint8_t pos_begin;
  uint8_t cur_flags;
};

/**
 * @given sr25519 provider instance configured with boost random generator
 * @when generate 2 keypairs, repeat it 10 times
 * @then each time keys are different
 */
TEST_F(Sr25519ProviderTest, GenerateKeysNotEqual) {
  VRFThreshold threshold{std::numeric_limits<VRFThreshold>::max() - 1};

  Blob b{std::array<uint8_t, 32>{212, 53, 147, 199, 21,  253, 211, 28,
                                 97,  20, 26,  189, 4,   169, 159, 214,
                                 130, 44, 133, 88,  133, 76,  205, 227,
                                 154, 86, 132, 231, 165, 109, 162, 125}};
  BabeSessionKey pk(b);
  Randomness r(std::array<uint8_t, 32>{
      0x86, 0xf6, 0x9b, 0x2e, 0x5a, 0xc5, 0xa9, 0x0f, 0x82, 0xc1, 0x82,
      0xda, 0x65, 0x94, 0x1c, 0x70, 0x25, 0x06, 0x8d, 0x45, 0xa7, 0x51,
      0x0a, 0x64, 0x19, 0xe6, 0xf1, 0x51, 0x2e, 0x2c, 0x04, 0xc0});
  BabeSlotNumber sn = 535781527;
  BabeSlotNumber ei = 9;

  VRFOutput vo{
      .output{90,  197, 90,  54,  39,  215, 26,  248, 213, 138, 141,
              169, 238, 94,  198, 250, 216, 160, 76,  238, 196, 170,
              249, 127, 163, 76,  205, 250, 133, 141, 223, 127},
      .proof{225, 51,  43,  182, 155, 142, 246, 74,  49,  66,  174, 209, 26,
             32,  57,  58,  8,   18,  105, 206, 101, 221, 0,   75,  222, 158,
             81,  230, 157, 42,  9,   4,   16,  152, 74,  82,  85,  68,  227,
             66,  96,  146, 248, 108, 239, 87,  66,  82,  130, 83,  4,   197,
             204, 200, 20,  254, 206, 16,  49,  162, 122, 2,   124, 14}};

  Transcript t;
  makeTranscript(t, r, sn, ei);

  auto seed =
      Blob<32>::fromHexWithPrefix(
          "0xe5be9a5092b81bca64be81d212e7f2f9eba183bb7a90954f7b76361f6edb5c0a")
          .value();
  auto keypair = sr25519_provider->generateKeypair(seed);

  Buffer keypair_buf{};
  keypair_buf.put(keypair.secret_key).put(keypair.public_key);

  ThinTranscript t_out;
  std::memcpy(t_out.data, t.data().data(), 200);
  t_out.pos = 144;
  t_out.pos_begin = 111;
  t_out.cur_flags = 2;

  std::array<uint8_t, 32 + 64> out_proof{};
  auto threshold_bytes = kagome::common::uint128_t_to_bytes(threshold);
  auto sign_res = sr25519_vrf_sign_test(
      out_proof.data(), keypair_buf.data(), (uint8_t*)&t_out, threshold_bytes.data());

  VRFOutput res;
  std::copy_n(
      out_proof.begin(), 32, res.output.begin());
  std::copy_n(out_proof.begin() + 32,
              64,
              res.proof.begin());

  auto q = t.data();
  std::memcpy(t_out.data, t.data().data(), 200);
  t_out.pos = 144;
  t_out.pos_begin = 111;
  t_out.cur_flags = 2;

  auto res4 =
      vrf_provider->verify(Buffer{q}, res, keypair.public_key, threshold);


  std::memcpy(t_out.data, t.data().data(), 200);
  t_out.pos = 144;
  t_out.pos_begin = 111;
  t_out.cur_flags = 2;

  auto r2 = sr25519_vrf_verify_test(keypair.public_key.data(),
                                (uint8_t*)&t_out,
                                res.output.data(),
                                res.proof.data(),
                                kagome::common::uint128_t_to_bytes(threshold).data());

  if (auto result = vrf_provider->sign(Buffer{q}, keypair, threshold)) {
    auto res2 = std::move(result.value());
    auto res3 =
        vrf_provider->verify(Buffer{q}, res2, keypair.public_key, threshold);
    int p = 0;
    ++p;
  }

  /*auto out_opt = vrf_provider_->sign(msg_, keypair1_, threshold);
  ASSERT_TRUE(out_opt);
  auto out = out_opt.value();


  auto verify_res__ = vrf_provider_->verify(
      Buffer(t.data()), vo, pk, threshold);
  if (not verify_res__.is_valid) {
    BOOST_ASSERT(!"BAD!");
  }
*/

  for (auto i = 0; i < 10; ++i) {
    auto kp1 = sr25519_provider->generateKeypair();
    auto kp2 = sr25519_provider->generateKeypair();
    ASSERT_NE(kp1.public_key, kp2.public_key);
    ASSERT_NE(kp1.secret_key, kp2.secret_key);
  }
}

/**
 * @given sr25519 provider instance configured with boost random generator
 * @and a predefined message
 * @when generate a keypair @and sign message
 * @and verify signed message with generated public key
 * @then verification succeeds
 */
TEST_F(Sr25519ProviderTest, SignVerifySuccess) {
  auto kp = sr25519_provider->generateKeypair();
  EXPECT_OUTCOME_TRUE(signature, sr25519_provider->sign(kp, message_span));
  EXPECT_OUTCOME_TRUE(
      res, sr25519_provider->verify(signature, message_span, kp.public_key));
  ASSERT_EQ(res, true);
}

/**
 * Don't try to sign a message using invalid key pair, this may lead to
 * program termination
 *
 * @given sr25519 provider instance configured with boost random generator
 * @and a predefined message
 * @when generate a keypair @and make public key invalid @and sign message
 * @then sign fails
 */
TEST_F(Sr25519ProviderTest, DISABLED_SignWithInvalidKeyFails) {
  auto kp = sr25519_provider->generateKeypair();
  kp.public_key.fill(1);
  EXPECT_OUTCOME_FALSE_1(sr25519_provider->sign(kp, message_span));
}

/**
 * @given sr25519 provider instance configured with boost random generator
 * @and and a predefined message
 * @when generate keypair @and sign message @and take another public key
 * @and verify signed message
 * @then verification succeeds, but verification result is false
 */
TEST_F(Sr25519ProviderTest, VerifyWrongKeyFail) {
  auto kp = sr25519_provider->generateKeypair();
  EXPECT_OUTCOME_TRUE(signature, sr25519_provider->sign(kp, message_span));
  // generate another valid key pair and take public one
  auto kp1 = sr25519_provider->generateKeypair();
  EXPECT_OUTCOME_TRUE(
      ver_res,
      sr25519_provider->verify(signature, message_span, kp1.public_key));

  ASSERT_FALSE(ver_res);
}

/**
 * Don't try to verify a message and signature against an invalid key, this may
 * lead to program termination
 *
 * @given sr25519 provider instance configured with boost random generator
 * @and and a predefined message
 * @when generate keypair @and sign message
 * @and generate another keypair and take public part for verification
 * @and verify signed message
 * @then verification fails
 */
TEST_F(Sr25519ProviderTest, DISABLED_VerifyInvalidKeyFail) {
  auto kp = sr25519_provider->generateKeypair();
  EXPECT_OUTCOME_TRUE(signature, sr25519_provider->sign(kp, message_span));
  // make public key invalid
  kp.public_key.fill(1);
  EXPECT_OUTCOME_FALSE_1(
      sr25519_provider->verify(signature, message_span, kp.public_key));
}

/**
 * @given seed value
 * @when generate key pair by seed
 * @then verifying and secret keys come up with predefined values
 */
TEST_F(Sr25519ProviderTest, GenerateBySeedSuccess) {
  EXPECT_OUTCOME_TRUE(seed, Sr25519Seed::fromHex(hex_seed));
  EXPECT_OUTCOME_TRUE(public_key, Sr25519PublicKey::fromHex(hex_vk));

  // private key is the same as seed
  EXPECT_OUTCOME_TRUE(secret_key, Sr25519SecretKey::fromHex(hex_sk));

  auto &&kp = sr25519_provider->generateKeypair(seed);

  ASSERT_EQ(kp.secret_key, secret_key);
  ASSERT_EQ(kp.public_key, public_key);
}
