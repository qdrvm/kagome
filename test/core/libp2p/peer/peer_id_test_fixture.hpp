/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_ID_TEST_FIXTURE_HPP
#define KAGOME_PEER_ID_TEST_FIXTURE_HPP

#include <memory>

#include <gtest/gtest.h>
#include "core/libp2p/crypto/crypto_provider_mock.hpp"
#include "core/libp2p/crypto/private_key_mock.hpp"
#include "core/libp2p/crypto/public_key_mock.hpp"
#include "core/libp2p/multi/multibase_codec_mock.hpp"
#include "libp2p/peer/peer_id_factory.hpp"

/**
 * Joins variables and methods, useful for testing PeerId and its Factory
 */
class PeerIdTestFixture : public ::testing::Test {
 public:
  void SetUp() override {}

  const libp2p::crypto::CryptoProviderMock crypto{};
  const libp2p::multi::MultibaseCodecMock multibase{};
  const libp2p::peer::PeerIdFactory factory{multibase, crypto};

  /// must be a SHA-256 multihash; in this case, it's a hash of 'mystring'
  /// string
  const kagome::common::Buffer valid_id{
      0x12, 0x20, 0xBD, 0x3F, 0xF4, 0x75, 0x40, 0xB3, 0x1E, 0x62, 0xD4, 0xCA,
      0x6B, 0x07, 0x79, 0x4E, 0x5A, 0x88, 0x6B, 0x0F, 0x65, 0x5F, 0xC3, 0x22,
      0x73, 0x0F, 0x26, 0xEC, 0xD6, 0x5C, 0xC7, 0xDD, 0x5C, 0x90};
  const kagome::common::Buffer invalid_id{0x66, 0x43};

  /// premade buffers, used for several things
  const kagome::common::Buffer just_buffer1{0x12, 0x34};
  const kagome::common::Buffer just_buffer2{0x56, 0x78};

  /// string, which is used to be mockly-hashed
  const std::string just_string{"mystring"};

  std::shared_ptr<libp2p::crypto::PublicKeyMock> public_key_shp =
      std::make_shared<libp2p::crypto::PublicKeyMock>();
  std::shared_ptr<libp2p::crypto::PrivateKeyMock> private_key_shp =
      std::make_shared<libp2p::crypto::PrivateKeyMock>();
  std::unique_ptr<libp2p::crypto::PublicKeyMock> public_key_uptr =
      std::make_unique<libp2p::crypto::PublicKeyMock>();
  std::unique_ptr<libp2p::crypto::PrivateKeyMock> private_key_uptr =
      std::make_unique<libp2p::crypto::PrivateKeyMock>();

  /**
   * Sets up mock keys and buffers, such that a valid configuration is created
   */
  void setUpValid() {
    // make public key return what we want
    EXPECT_CALL(*public_key_shp, getBytes())
        .WillRepeatedly(testing::ReturnRef(just_buffer1));
    EXPECT_CALL(*public_key_shp, getType())
        .WillRepeatedly(
            testing::Return(libp2p::crypto::common::KeyType::kRSA1024));

    // make private key return what we want
    EXPECT_CALL(*private_key_shp, getBytes())
        .WillRepeatedly(testing::ReturnRef(just_buffer2));
    EXPECT_CALL(*private_key_shp, getType())
        .WillRepeatedly(
            testing::Return(libp2p::crypto::common::KeyType::kRSA1024));

    // make public_key_uptr equal to public_key
    EXPECT_CALL(*public_key_uptr, getBytes())
        .WillRepeatedly(testing::ReturnRef(public_key_shp->getBytes()));
    EXPECT_CALL(*public_key_uptr, getType())
        .WillRepeatedly(testing::Return(public_key_shp->getType()));

    // make private_key_uptr equal to private_key
    EXPECT_CALL(*private_key_uptr, getBytes())
        .WillRepeatedly(testing::ReturnRef(private_key_shp->getBytes()));
    EXPECT_CALL(*private_key_uptr, getType())
        .WillRepeatedly(testing::Return(private_key_shp->getType()));

    // make encode return such string, that after hashing it becomes valid_id
    EXPECT_CALL(multibase,
                encode(public_key_shp->getBytes(),
                       libp2p::multi::MultibaseCodec::Encoding::kBase64))
        .WillRepeatedly(testing::Return(just_string));
  }
};

#endif  // KAGOME_PEER_ID_TEST_FIXTURE_HPP
