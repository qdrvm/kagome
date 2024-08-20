/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <qtils/test/outcome.hpp>

#include "crypto/bandersnatch/bandersnatch_provider_impl.hpp"
#include "crypto/bandersnatch/vrf.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::common::Blob;
using kagome::common::Buffer;
using kagome::crypto::BandersnatchKeypair;
using kagome::crypto::BandersnatchProvider;
using kagome::crypto::BandersnatchProviderImpl;
using kagome::crypto::BandersnatchSeed;
using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::CSPRNG;
using kagome::crypto::Hasher;
using kagome::crypto::HasherImpl;
using kagome::crypto::SecureBuffer;

namespace vrf = kagome::crypto::bandersnatch::vrf;

struct BandersnatchTest : public ::testing::Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  static kagome::log::Logger log() {
    static auto logger = kagome::log::createLogger("test");
    return logger;
  }

  void SetUp() override {
    hasher = std::make_shared<HasherImpl>();
    random_generator = std::make_shared<BoostRandomGenerator>();
    bandersnatch_provider = std::make_shared<BandersnatchProviderImpl>(hasher);

    message = "I am a message"_bytes;
  }

  auto generate() {
    SecureBuffer<> seed_buf(BandersnatchSeed::size());
    random_generator->fillRandomly(seed_buf);
    auto seed = BandersnatchSeed::from(std::move(seed_buf)).value();
    return bandersnatch_provider->generateKeypair(seed, {});
  }

  std::span<const uint8_t> message;

  std::shared_ptr<Hasher> hasher;
  std::shared_ptr<CSPRNG> random_generator;
  std::shared_ptr<BandersnatchProvider> bandersnatch_provider;
};

/**
 * @given
 * @when
 * @then
 */
TEST_F(BandersnatchTest, GenerateKeysNotEqual) {
  for (auto i = 0; i < 10; ++i) {
    auto kp1 = EXPECT_OK(generate());
    auto kp2 = EXPECT_OK(generate());
    ASSERT_NE(kp1.public_key, kp2.public_key);
    ASSERT_NE(kp1.secret_key, kp2.secret_key);
  }
}

/**
 * @given
 * @when
 * @then
 */
TEST_F(BandersnatchTest, PlainSignVerifySuccess) {
  auto kp = EXPECT_OK(generate());
  auto signature = EXPECT_OK(bandersnatch_provider->sign(kp, message));
  auto is_valid = EXPECT_OK(
      bandersnatch_provider->verify(signature, message, kp.public_key));
  ASSERT_TRUE(is_valid);
}

/**
 * @given
 * @when
 * @then
 */
TEST_F(BandersnatchTest, VrfSignVerifySuccess) {
  vrf::Bytes labels[] = {
      Buffer::fromString("label_one"),
      // Buffer::fromString("label_two"),
      // Buffer::fromString("label_three"),
      // Buffer::fromString("label_four"),
  };

  vrf::Bytes tds[] = {
      Buffer::fromString("transcript_one"),
      Buffer::fromString("transcript_two"),
      // Buffer::fromString("transcript_three"),
      // Buffer::fromString("transcript_four"),
  };

  vrf::Bytes ins[] = {
      Buffer::fromString("input_one"), Buffer::fromString("input_two"),
      // Buffer::fromString("input_three"),
  };

  for (auto i = 0; i < 3; ++i) {
    auto kp = EXPECT_OK(generate());

    SL_INFO(log(), "PUB={}", kp.public_key);
    for (auto &label : labels) {
      SL_INFO(log(), "  LABEL={}", Buffer(label).asString());
      for (auto tds_n = 0u; tds_n <= std::size(tds); ++tds_n) {
        std::vector<vrf::BytesIn> td;
        for (auto tds_i = 0u; tds_i < tds_n; ++tds_i) {
          td.emplace_back(tds[tds_i]);
        }
        SL_INFO(log(), "    TRANSCRIPT={}", td.size());
        for (auto ins_n = 0u; ins_n <= std::size(ins); ++ins_n) {
          std::vector<vrf::VrfInput> inputs;
          for (auto ins_i = 0u; ins_i < ins_n; ++ins_i) {
            std::vector<vrf::BytesIn> data{ins[ins_i]};
            auto input = vrf::vrf_input_from_data("domain"_bytes, data);
            inputs.emplace_back(std::move(input));
          }
          SL_INFO(log(), "      INPUTS={}", inputs.size());
          SL_INFO(log(),
                  "        kp={} label={} td={} ins={}",
                  kp.public_key,
                  Buffer(label).asString(),
                  td.size(),
                  inputs.size());

          auto sign_data = vrf::vrf_sign_data(label, td, inputs);

          auto sign = vrf::vrf_sign(kp.secret_key, sign_data);

          auto is_valid = vrf::vrf_verify(sign, sign_data, kp.public_key);
          ASSERT_TRUE(is_valid);
        }
      }
    }
  }
}
