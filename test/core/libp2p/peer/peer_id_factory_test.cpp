/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include <gtest/gtest.h>
#include "core/libp2p/crypto/crypto_provider_mock.hpp"
#include "core/libp2p/crypto/private_key_mock.hpp"
#include "core/libp2p/crypto/public_key_mock.hpp"
#include "core/libp2p/multi/multibase_codec_mock.hpp"
#include "libp2p/peer/peer_id_factory.hpp"

using namespace libp2p::crypto;
using namespace libp2p::peer;
using namespace libp2p::multi;
using namespace kagome::common;

using testing::ByMove;
using testing::Return;
using testing::ReturnRef;

class PeerIdFactoryTest : public ::testing::Test {
 public:
  const CryptoProviderMock crypto{};
  const MultibaseCodecMock multibase{};
  const PeerIdFactory factory{multibase, crypto};

  /// must be a SHA-256 multihash; in this case, it's a hash of 'mystring'
  /// string
  const Buffer valid_id{0x12, 0x20, 0xBD, 0x3F, 0xF4, 0x75, 0x40, 0xB3, 0x1E,
                        0x62, 0xD4, 0xCA, 0x6B, 0x07, 0x79, 0x4E, 0x5A, 0x88,
                        0x6B, 0x0F, 0x65, 0x5F, 0xC3, 0x22, 0x73, 0x0F, 0x26,
                        0xEC, 0xD6, 0x5C, 0xC7, 0xDD, 0x5C, 0x90};
  const Buffer invalid_id{0x66, 0x43};
  const Buffer just_buffer{0x12, 0x34};

  std::shared_ptr<PublicKeyMock> public_key = std::make_shared<PublicKeyMock>();
  std::shared_ptr<PrivateKeyMock> private_key =
      std::make_shared<PrivateKeyMock>();
  std::unique_ptr<PublicKeyMock> public_key_ptr =
      std::make_unique<PublicKeyMock>();

  void setUpValid() {
    ON_CALL(*public_key, getBytes()).WillByDefault(ReturnRef(just_buffer));
    ON_CALL(*public_key, getType())
        .WillByDefault(Return(common::KeyType::kRSA1024));

    // make public_key_ptr equal to public_key
    ON_CALL(*public_key_ptr, getBytes())
        .WillByDefault(ReturnRef(public_key->getBytes()));
    ON_CALL(*public_key_ptr, getType())
        .WillByDefault(Return(public_key->getType()));

    ON_CALL(*private_key, publicKey())
        .WillByDefault(Return(ByMove(std::move(public_key_ptr))));
    ON_CALL(multibase,
            encode(public_key->getBytes(), MultibaseCodec::Encoding::kBase64))
        .WillByDefault(Return("mystring"));
  }
};

/**
 * @given initialized factory @and valid peer id in bytes
 * @when creating PeerId from the bytes
 * @then creation succeeds
 */
TEST_F(PeerIdFactoryTest, FromBufferSuccess) {
  auto result = factory.createPeerId(valid_id);

  ASSERT_TRUE(result.hasValue());
  const auto &peer_id = result.getValueRef();
  ASSERT_EQ(peer_id.toBytes(), valid_id);
  ASSERT_FALSE(peer_id.publicKey());
  ASSERT_FALSE(peer_id.privateKey());
}

/**
 * @given initialized factory @and invalid peer id in bytes
 * @when creating PeerId from the bytes
 * @then creation fails
 */
TEST_F(PeerIdFactoryTest, FromBufferWrongBuffer) {
  auto result = factory.createPeerId(invalid_id);

  ASSERT_FALSE(result.hasValue());
}

TEST_F(PeerIdFactoryTest, FromBufferKeysSuccess) {
  setUpValid();

  auto result = factory.createPeerId(valid_id, public_key, private_key);

  ASSERT_TRUE(result.hasValue());
  const auto &peer_id = result.getValueRef();
  ASSERT_EQ(peer_id.toBytes(), valid_id);
  ASSERT_EQ(*peer_id.publicKey(), *public_key);
  ASSERT_EQ(*peer_id.privateKey(), *private_key);
}

TEST_F(PeerIdFactoryTest, FromBufferKeysEmptyBuffer) {}

TEST_F(PeerIdFactoryTest, FromBufferKeysWrongKeys) {}

TEST_F(PeerIdFactoryTest, FromPubkeyObjectSuccess) {}

TEST_F(PeerIdFactoryTest, FromPrivkeyObjectSuccess) {}

TEST_F(PeerIdFactoryTest, FromPubkeyBufferSuccess) {}

TEST_F(PeerIdFactoryTest, FromPubkeyBufferWrongKey) {}

TEST_F(PeerIdFactoryTest, FromPrivkeyBufferSuccess) {}

TEST_F(PeerIdFactoryTest, FromPrivkeyBufferWrongKey) {}

TEST_F(PeerIdFactoryTest, FromPubkeyStringSuccess) {}

TEST_F(PeerIdFactoryTest, FromPubkeyStringWrongKey) {}

TEST_F(PeerIdFactoryTest, FromPrivkeyStringSuccess) {}

TEST_F(PeerIdFactoryTest, FromPrivkeyStringWrongKey) {}

TEST_F(PeerIdFactoryTest, FromEncodedStringSuccess) {}

TEST_F(PeerIdFactoryTest, FromEncodedBadEncoding) {}
