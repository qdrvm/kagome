/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multiaddress.hpp"
#include <gtest/gtest.h>

using libp2p::multi::Multiaddress;

using namespace kagome::expected;
using namespace kagome::common;

class MultiaddressTest : public ::testing::Test {
 public:
  const std::string_view valid_ip_udp_address = "/ip4/192.168.0.1/udp/228/";
  const std::vector<uint8_t> valid_ip_udp_bytes{
      0x4, 0xC0, 0xA8, 0x0, 0x1, 0x11, 0x0, 0xE4};
  const Buffer valid_id_udp_buffer{valid_ip_udp_bytes};

  const std::string_view valid_ip_address = "/ip4/192.168.0.1/";
  const std::string_view valid_ipfs_address = "/ipfs/mypeer/";

  const std::string_view invalid_address = "/ip4/192.168.0.1/2/";
  const std::vector<uint8_t> invalid_bytes{0x4, 0xC0, 0xA8, 0x0, 0x1, 0x02};
  const Buffer invalid_buffer{invalid_bytes};

  /**
   * Create a valid multiaddress
   * @param string_address of the address; MUST be valid
   * @return pointer to address
   */
  std::unique_ptr<Multiaddress> createValidMultiaddress(
      std::string_view string_address = "/ip4/192.168.0.1/udp/228/") {
    return std::move(Multiaddress::createMultiaddress(string_address).getValue());
  }
};

/**
 * @given valid string address
 * @when creating a multiaddress from it
 * @then creation succeeds
 */
TEST_F(MultiaddressTest, CreateFromStringValid) {
  auto address_res = Multiaddress::createMultiaddress(valid_ip_udp_address);
  ASSERT_TRUE(address_res.hasValue());
  auto& address = address_res.getValueRef();
  ASSERT_EQ(address->getStringAddress(), valid_ip_udp_address);
  ASSERT_EQ(address->getBytesAddress(), valid_ip_udp_bytes);
}

/**
 * @given invalid string address
 * @when creating a multiaddress from it
 * @then creation fails
 */
TEST_F(MultiaddressTest, CreateFromStringInvalid) {
  auto address = Multiaddress::createMultiaddress(invalid_address);
  ASSERT_FALSE(address.hasValue());
}

/**
 * @given valid byte address
 * @when creating a multiaddress from it
 * @then creation succeeds
 */
TEST_F(MultiaddressTest, CreateFromBytesValid) {
  auto address = Multiaddress::createMultiaddress(valid_id_udp_buffer);
  ASSERT_TRUE(address.hasValue());
  auto v = address.getValue();
  ASSERT_EQ(v->getStringAddress(), valid_ip_udp_address);
  ASSERT_EQ(v->getBytesAddress(), valid_ip_udp_bytes);
}

/**
 * @given invalid string address
 * @when creating a multiaddress from it
 * @then creation fails
 */
TEST_F(MultiaddressTest, CreateFromBytesInvalid) {
  auto address = Multiaddress::createMultiaddress(invalid_buffer);
  ASSERT_FALSE(address.hasValue());
}

/**
 * @given two valid multiaddresses
 * @when encapsulating one of them to another
 * @then encapsulated address' string and bytes representations are equal to
 * manually joined ones @and address, created from the joined string, is the
 * same, as the encapsulated one
 */
TEST_F(MultiaddressTest, Incapsulate) {
  auto address1 = createValidMultiaddress();
  auto address2 = createValidMultiaddress(valid_ipfs_address);

  auto joined_string_address = std::string{valid_ip_udp_address}
      + std::string{valid_ipfs_address.substr(1)};
  auto joined_byte_address = address1->getBytesAddress().toVector();
  joined_byte_address.insert(joined_byte_address.end(),
                             address2->getBytesAddress().begin(),
                             address2->getBytesAddress().end());

  address1->encapsulate(*address2);
  ASSERT_EQ(address1->getStringAddress(), joined_string_address);
  ASSERT_EQ(address1->getBytesAddress(), joined_byte_address);

  auto joined_address = Multiaddress::createMultiaddress(joined_string_address);
  ASSERT_TRUE(joined_address.hasValue());
  ASSERT_EQ(*joined_address.getValue(), *address1);
}

/**
 * @given valid multiaddress
 * @when decapsulating it with another address, which contains part of the
 * initial one
 * @then decapsulation is successful
 */
TEST_F(MultiaddressTest, DecapsulateValid) {
  auto initial_address = createValidMultiaddress();
  auto address_to_decapsulate = createValidMultiaddress("/udp/228/");
  auto decapsulated_address = createValidMultiaddress(valid_ip_address);

  ASSERT_TRUE(initial_address->decapsulate(*address_to_decapsulate));
  ASSERT_EQ(*initial_address, *decapsulated_address);
}

/**
 * @given valid multiaddress
 * @when decapsulating it with another address, which does not ontain part of
 * the initial one
 * @then decapsulation fails
 */
TEST_F(MultiaddressTest, DecapsulateInvalid) {
  auto initial_address = createValidMultiaddress();
  auto address_to_decapsulate = createValidMultiaddress(valid_ipfs_address);

  ASSERT_FALSE(initial_address->decapsulate(*address_to_decapsulate));
}

/**
 * @given valid multiaddress
 * @when getting its string representation
 * @then result is equal to the expected one
 */
TEST_F(MultiaddressTest, GetString) {
  auto address = createValidMultiaddress();
  ASSERT_EQ(address->getStringAddress(), valid_ip_udp_address);
}

/**
 * @given valid multiaddress
 * @when getting its byte representation
 * @then result is equal to the expected one
 */
TEST_F(MultiaddressTest, GetBytes) {
  auto address = createValidMultiaddress();
  ASSERT_EQ(address->getBytesAddress(), valid_ip_udp_bytes);
}

/**
 * @given valid multiaddress with IPFS part
 * @when getting peer id
 * @then it exists @and is equal to the expected one
 */
TEST_F(MultiaddressTest, GetPeerIdExists) {
  auto address = createValidMultiaddress(valid_ipfs_address);
  ASSERT_EQ(*address->getPeerId(), "mypeer");
}

/**
 * @given valid multiaddress without IPFS part
 * @when getting peer id
 * @then it does not exist
 */
TEST_F(MultiaddressTest, GetPeerIdNotExists) {
  auto address = createValidMultiaddress();
  ASSERT_FALSE(address->getPeerId());
}

/**
 * @given valid multiaddress
 * @when getting values for protocols, which exist in this multiaddress
 * @then they are returned
 */
TEST_F(MultiaddressTest, GetValueForProtocolValid) {
  auto address =
      createValidMultiaddress(std::string{valid_ip_udp_address} + "udp/432/");

  auto protocols = address->getValuesForProtocol(Multiaddress::Protocol::kUdp);
  ASSERT_TRUE(protocols);
  ASSERT_EQ(protocols->at(0), "228");
  ASSERT_EQ(protocols->at(1), "432");
}

/**
 * @given valid multiaddress
 * @when getting values for protocols, which does not exist in this multiaddress
 * @then result is empty
 */
TEST_F(MultiaddressTest, GetValueForProtocolInvalid) {
  auto address = createValidMultiaddress();

  auto protocols =
      address->getValuesForProtocol(Multiaddress::Protocol::kOnion);
  ASSERT_FALSE(protocols);
}
