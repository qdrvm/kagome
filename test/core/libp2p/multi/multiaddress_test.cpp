/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multiaddress.hpp"
#include <gtest/gtest.h>
#include "utils/result_fixture.hpp"

using libp2p::multi::Multiaddress;

using namespace kagome::expected;
using namespace kagome::expected::testing;
using namespace kagome::common;

class MultihashTest : public ::testing::Test {
 public:
  static constexpr std::string_view valid_address = "/ip4/192.168.0.1/udp/228";
  const Buffer valid_buffer = Buffer{}.put(valid_address.data());
  const std::vector<uint8_t> &valid_bytes = valid_buffer.to_vector();

  static constexpr std::string_view invalid_address =
      "/ip4/192.168.0.1/udp/tcp";
  const Buffer invalid_buffer = Buffer{}.put(invalid_address.data());
  const std::vector<uint8_t> &invalid_bytes = invalid_buffer.to_vector();
};

/**
 * @given valid string address
 * @when creating a multiaddress from it
 * @then creation succeeds
 */
TEST_F(MultihashTest, CreateFromStringValid) {
  auto address = val(Multiaddress::createMultiaddress(valid_address));
  ASSERT_TRUE(address);
  ASSERT_EQ(address->value->getStringAddress(), valid_address);
  ASSERT_EQ(address->value->getBytesAddress(), valid_bytes);
}

/**
 * @given invalid string address
 * @when creating a multiaddress from it
 * @then creation fails
 */
TEST_F(MultihashTest, CreateFromStringInvalid) {
  auto address = err(Multiaddress::createMultiaddress(invalid_address));
  ASSERT_TRUE(address);
}

/**
 * @given valid byte address
 * @when creating a multiaddress from it
 * @then creation succeeds
 */
TEST_F(MultihashTest, CreateFromBytesValid) {
  auto address = val(
      Multiaddress::createMultiaddress(std::make_shared<Buffer>(valid_buffer)));
  ASSERT_TRUE(address);
  ASSERT_EQ(address->value->getStringAddress(), valid_address);
  ASSERT_EQ(address->value->getBytesAddress(), valid_bytes);
}

/**
 * @given invalid string address
 * @when creating a multiaddress from it
 * @then creation fails
 */
TEST_F(MultihashTest, CreateFromBytesInvalid) {
  auto address = err(Multiaddress::createMultiaddress(
      std::make_shared<Buffer>(invalid_buffer)));
  ASSERT_TRUE(address);
}

//TEST_F(MultihashTest, Incapsulate);
//
//TEST_F(MultihashTest, DecapsulateValid);
//
//TEST_F(MultihashTest, DecapsulateInvalid);
//
//TEST_F(MultihashTest, GetString);
//
//TEST_F(MultihashTest, GetBytes);
//
//TEST_F(MultihashTest, GetPeerIdExists);
//
//TEST_F(MultihashTest, GetPeerIdNotExists);
//
//TEST_F(MultihashTest, GetValueForProtocolValid);
//
//TEST_F(MultihashTest, GetValueForProtocolInvalid);
