/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/random/impl/boost_generator.hpp"
#include "libp2p/crypto/random/impl/std_generator.hpp"

#include <iostream>

#include <gtest/gtest.h>

using kagome::common::Buffer;
using libp2p::crypto::random::BoostRandomGenerator;
using libp2p::crypto::random::StdRandomGenerator;

/**
 * @given 2 boost random numbers generators
 * @when randomBytes is called to generate to buffers of random nubmers
 * @then obtained byte sequences are not equal
 */
TEST(Random, boostGenerator) {
  BoostRandomGenerator generator1, generator2;
  constexpr size_t BUFFER_SIZE = 32;
  std::vector<unsigned char> bytes1(BUFFER_SIZE, 0);
  std::vector<unsigned char> bytes2(BUFFER_SIZE, 0);

  generator1.randomBytes(bytes1.data(), BUFFER_SIZE);
  generator2.randomBytes(bytes2.data(), BUFFER_SIZE);

  std::cout << Buffer(bytes1).toHex() << std::endl;
  std::cout << Buffer(bytes2).toHex() << std::endl;
  ASSERT_NE(bytes1, bytes2);
}

/**
 * @given 2 std random numbers generators
 * @when randomBytes is called to generate to buffers of random nubmers
 * @then obtained byte sequences are not equal
 */
TEST(Random, stdGenerator) {
  StdRandomGenerator generator1, generator2;
  constexpr size_t BUFFER_SIZE = 32;
  std::vector<unsigned char> bytes1(BUFFER_SIZE, 0);
  std::vector<unsigned char> bytes2(BUFFER_SIZE, 0);

  generator1.randomBytes(bytes1.data(), BUFFER_SIZE);
  generator2.randomBytes(bytes2.data(), BUFFER_SIZE);

  std::cout << Buffer(bytes1).toHex() << std::endl;
  std::cout << Buffer(bytes2).toHex() << std::endl;
  ASSERT_NE(bytes1, bytes2);
}

namespace {
  /**
   * Taken from
   * https://github.com/hyperledger/iroha-ed25519/blob/master/test/randombytes/random.cpp#L6
   * @brief calculates entropy of byte sequence
   * @param sequence  source byte sequence
   * @return entropy value
   * @
   */
  double entropy(gsl::span<uint8_t> sequence) {
    std::vector<uint8_t> freqs(256, 0);

    // calculate frequencies
    for (unsigned char i : sequence) {
      ++freqs[i];
    }

    double e = 0;
    for (const auto c : freqs) {
      double freq = (double)c / sequence.size();

      if (freq == 0) {
        continue;
      }

      e += freq * log2(freq);
    }

    return -e;
  }

  /**
   * @brief returns max possible entropy for a buffer of given size
   * @param volume is volume of alphabet
   * @return max value of entropy for given source alphabet volume
   */
  double max_entropy(size_t volume) {
    return log2(volume);
  }
}  // namespace

/**
 * @given BoostRandomGenerator instance, max entropy value for given source
 * @when get random bytes and calculate entropy
 * @then calculated entropy is not less than (max entropy - 2)
 */
TEST(Random, enoughEntropyBoost) {
  BoostRandomGenerator generator;
  std::vector<uint8_t> buf(256, 0);
  generator.randomBytes(buf.data(), buf.size());

  auto max = max_entropy(256);  // 8
  auto ent = entropy(buf);

  std::cout << "max entropy: " << max << std::endl;
  std::cout << "calculated:  " << ent << std::endl;

  ASSERT_GE(ent, max - 2) << "bad randomness source";
}

/**
 * @given StdRandomGenerator instance, max entropy value for given source
 * @when get random bytes and calculate entropy
 * @then calculated entropy is not less than (max entropy - 2)
 */
TEST(Random, enoughEntropyStd) {
  StdRandomGenerator generator;
  std::vector<uint8_t> buf(256, 0);
  generator.randomBytes(buf.data(), buf.size());

  auto max = max_entropy(256);  // 8
  auto ent = entropy(buf);

  std::cout << "max entropy: " << max << std::endl;
  std::cout << "calculated:  " << ent << std::endl;

  ASSERT_GE(ent, max - 2) << "bad randomness source";
}
