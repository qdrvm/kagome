/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <span>

#include "crypto/bandersnatch/bandersnatch_provider_impl.hpp"
#include "crypto/bandersnatch/vrf.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::crypto::BandersnatchKeypair;
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

namespace vrf = kagome::crypto::bandersnatch::vrf;

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

    message = "I am a message"_bytes;

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

/**
 * @given
 * @when
 * @then
 */
TEST_F(BandersnatchTest, GenerateKeysNotEqual) {
  for (auto i = 0; i < 10; ++i) {
    auto kp1 = generate();
    auto kp2 = generate();
    ASSERT_NE(kp1.public_key, kp2.public_key);
    ASSERT_NE(kp1.secret_key, kp2.secret_key);
  }
}

TEST_F(BandersnatchTest, PlainSignVerifySuccess) {
  auto kp = generate();
  ASSERT_OUTCOME_SUCCESS(signature, bandersnatch_provider->sign(kp, message));
  EXPECT_OUTCOME_SUCCESS(
      is_valid,
      bandersnatch_provider->verify(signature, message, kp.public_key));
  ASSERT_TRUE(is_valid);
}

TEST_F(BandersnatchTest, VrfSignVerifySuccess) {
  auto examinee = [](const BandersnatchKeypair &kp,
                     libp2p::BytesIn label,
                     std::span<libp2p::BytesIn> transcript_data,
                     std::span<vrf::VrfInput> inputs) {
    SL_INFO(log(),
            "        kp={} label={} td={} ins={}",
            kp.public_key,
            kagome::common::Buffer(label).asString(),
            transcript_data.size(),
            inputs.size());

    auto data = vrf::vrf_sign_data(label, transcript_data, inputs);

    auto sign = vrf::vrf_sign(kp.secret_key, data);

    auto is_valid = vrf::vrf_verify(sign, data, kp.public_key);
    return is_valid;
  };

  std::array<vrf::Bytes, 4> labels = {
      kagome::common::Buffer::fromString("label_one"),
      kagome::common::Buffer::fromString("label_two"),
      kagome::common::Buffer::fromString("label_three"),
      kagome::common::Buffer::fromString("label_four"),
  };

  std::array<vrf::Bytes, 4> tds = {
      kagome::common::Buffer::fromString("transcript_one"),
      kagome::common::Buffer::fromString("transcript_two"),
      kagome::common::Buffer::fromString("transcript_three"),
      kagome::common::Buffer::fromString("transcript_four"),
  };

  std::array<vrf::Bytes, 3> ins = {
      kagome::common::Buffer::fromString("input_one"),
      kagome::common::Buffer::fromString("input_two"),
      kagome::common::Buffer::fromString("input_three"),
  };

  auto by_inputs = [&](const BandersnatchKeypair &kp,
                       libp2p::BytesIn label,
                       std::span<libp2p::BytesIn> td) {
    SL_INFO(log(), "    TRANSCRIPT={}", td.size());
    for (auto n = 0u; n <= ins.size(); ++n) {
      std::vector<vrf::VrfInput> inputs;
      for (auto i = 0u; i < n; ++i) {
        std::vector<vrf::BytesIn> data{ins[i]};
        auto input = vrf::vrf_input_from_data("domain"_bytes, data);
        inputs.emplace_back(input);
      }
      SL_INFO(log(), "      INPUTS={}", inputs.size());
      if (!examinee(kp, label, td, inputs)) {
        return false;
      };
    }
    return true;
  };

  auto by_transcript = [&](const BandersnatchKeypair &kp,
                           libp2p::BytesIn label) {
    SL_INFO(log(), "  LABEL={}", kagome::common::Buffer(label).asString());
    for (auto n = 0u; n <= tds.size(); ++n) {
      std::vector<vrf::BytesIn> td;
      for (auto i = 0u; i < n; ++i) {
        td.emplace_back(tds[i]);
      }
      if (!by_inputs(kp, label, td)) {
        return false;
      }
    }
    return true;
  };

  auto by_label = [&](const BandersnatchKeypair &kp) {
    SL_INFO(log(), "PUB={}", kp.public_key);
    for (auto &label : labels) {
      if (!by_transcript(kp, label)) {
        return false;
      }
    }
    return true;
  };

  for (const BandersnatchKeypair &kp : {generate(), generate(), generate()}) {
    ASSERT_TRUE(by_label(kp));
  }
}
