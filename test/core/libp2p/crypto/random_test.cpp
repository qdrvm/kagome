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
