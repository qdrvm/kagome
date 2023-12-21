/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <span>

#include "crypto/bandersnatch/bandersnatch_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::crypto::BandersnatchProvider;
using kagome::crypto::BandersnatchProviderImpl;
using kagome::crypto::BandersnatchSeed;
using kagome::crypto::BoostRandomGenerator;
// namespace bandersnatch = kagome::crypto::bandersnatch;
// namespace vrf = bandersnatch::vrf;
//
// using vrf::kMaxVrfInputOutputCounts;
// using vrf::VrfInput;
// using vrf::VrfOutput;
// using vrf::VrfSignData;
//
// using bandersnatch::BytesIn;
// using bandersnatch::BytesOut;

using kagome::common::Blob;

struct BandersnatchTest : public ::testing::Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  static kagome::log::Logger log() {
    static auto logger = kagome::log::createLogger("test");
    return logger;
  }

  void SetUp() override {
    bandersnatch_provider = std::make_shared<BandersnatchProviderImpl>();

    message = "i am a message"_bytes;

    //    hex_seed =
    // "ccb4ec79974db3dae0d4dff7e0963db6b798684356dc517ff5c2e61f3b641569";
    //    hex_public_key =
    // "767a2f677a8c704d66e2abbb181d8984adae7ac8ecac9e30709ad496244ab497";
  }

  auto generate() {
    BandersnatchSeed seed;
    csprng->fillRandomly(seed);
    return bandersnatch_provider->generateKeypair(seed, {});
  }

  //  std::string_view hex_seed;
  //  std::string_view hex_public_key;
  //
  std::span<const uint8_t> message;

  std::shared_ptr<BoostRandomGenerator> csprng =
      std::make_shared<BoostRandomGenerator>();
  //  std::shared_ptr<HasherImpl> hasher = std::make_shared<HasherImpl>();
  //  std::shared_ptr<Ed25519Provider> ed25519_provider;
  std::shared_ptr<BandersnatchProvider> bandersnatch_provider;
};

kagome::log::Logger log_;

/**
 * @given ed25519 provider instance configured with boost random generator
 * @when generate 2 keypairs, repeat it 10 times
 * @then each time keys are different
 */
TEST_F(BandersnatchTest, GenerateKeysNotEqual) {
  for (auto i = 0; i < 10; ++i) {
    auto kp1 = generate();
    auto kp2 = generate();
    ASSERT_NE(kp1.public_key, kp2.public_key);
    ASSERT_NE(kp1.secret_key, kp2.secret_key);
  }
}

/**
 * @given ed25519 provider instance configured with boost random generator
 * @and a predefined message
 * @when generate a keypair @and sign message
 * @and verify signed message with generated public key
 * @then verification succeeds
 */
TEST_F(BandersnatchTest, SignVerifySuccess) {
  auto kp = generate();
  ASSERT_OUTCOME_SUCCESS(signature, bandersnatch_provider->sign(kp, message));
  EXPECT_OUTCOME_SUCCESS(
      is_valid,
      bandersnatch_provider->verify(signature, message, kp.public_key));
  ASSERT_TRUE(is_valid);
}

// TEST_F(BandersnatchTest, createSecret) {
//   bandersnatch::Seed seed;
//   for (auto &e : seed) {
//     e = '\xab';
//   }
//
//   auto secret = bandersnatch::SecretKey(seed);
//
//   [[maybe_unused]] auto public_key = secret.to_public();
// }
//
// TEST_F(BandersnatchTest, generateKeypair) {
//   bandersnatch::Seed seed;
//   for (auto &e : seed) {
//     e = '\xab';
//   }
//   SL_INFO(log(), "SEED(orig): {:l}", (Blob<32> &)seed);
//
//   auto keypair = bandersnatch::Pair(seed);
//
//   [[maybe_unused]] auto seed_ = keypair.seed();
//   SL_INFO(log(), "SEED(keypair): {:l}", (Blob<32> &)seed_);
//
//   [[maybe_unused]] auto public_key = keypair.publicKey();
//   SL_INFO(log(), "PUB(keypair): {:l}", (Blob<33> &)public_key);
//
//   Blob<51> msg;
//   for (auto &m : msg) {
//     m = '\xef';
//   }
//   SL_INFO(log(), "MSG(orig): {:l}", msg);
//
//   auto sig = keypair.sign(msg);
//   SL_INFO(log(), "SIG: {:l}", (Blob<65> &)sig);
// }
//
// TEST_F(BandersnatchTest, max_vrf_ios_bound_respected) {
//   ;  //
//
//   {
//     // VrfInput each with empty domain and data
//     std::vector<VrfInput> inputs;
//
//     for (auto i = kMaxVrfInputOutputCounts - 1; i > 0; --i) {
//       inputs.emplace_back(BytesIn{}, BytesIn{});
//     }
//
//     //// VrfSignData with empty label, one empty transcript data and inputs
//     // kagome::common::Blob<0> empty_transcript_data;
//     // std::vector<BytesIn> transcript_data = {empty_transcript_data};
//     // VrfSignData sign_data{{}, transcript_data, inputs};
//     //
//     // VrfInput available_input{{}, {}};
//     // ASSERT_OUTCOME_SUCCESS_TRY(sign_data.push_vrf_input(available_input));
//     //
//     // VrfInput extra_input{{}, {}};
//     // ASSERT_OUTCOME_SOME_ERROR(sign_data.push_vrf_input(extra_input));
//   }
//
//   {
//     std::vector<VrfInput> inputs;
//     for (auto i = kMaxVrfInputOutputCounts - 1; i > 0; --i) {
//       inputs.emplace_back(BytesIn{}, BytesIn{});
//     }
//     // ASSERT_OUTCOME_SOME_ERROR(VrfSignData::cteate(""_bytes, ""_bytes,
//     // inputs));
//   }
// };
