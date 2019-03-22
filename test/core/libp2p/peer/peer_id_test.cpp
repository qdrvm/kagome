/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/peer_id.hpp"
#include <gtest/gtest.h>
#include "core/libp2p/peer/peer_id_test_fixture.hpp"

using namespace libp2p::peer;
using namespace libp2p::crypto;

using ::testing::ByMove;
using ::testing::Return;

class PeerIdTest : public PeerIdTestFixture {
 public:
  void SetUp() override {
    PeerIdTestFixture::SetUp();
    setUpValid();
  }

  /**
   * Create a PeerId with all valid params
   * @return created PeerId
   */
  PeerId createValidPeerId() {
    EXPECT_CALL(*private_key_shp, publicKey())
        .WillRepeatedly(Return(ByMove(std::move(public_key_uptr))));
    return factory.createPeerId(valid_id, public_key_shp, private_key_shp)
        .getValue();
  }
};

TEST_F(PeerIdTest, GetHex) {
  EXPECT_CALL(
      multibase,
      encode(valid_id, libp2p::multi::MultibaseCodec::Encoding::kBase16Lower))
      .WillOnce(Return(valid_id.toHex()));

  auto peer_id = createValidPeerId();

  ASSERT_EQ(peer_id.toHex(), valid_id.toHex());
}

TEST_F(PeerIdTest, GetBytes) {
  auto peer_id = createValidPeerId();

  ASSERT_EQ(peer_id.toBytes(), valid_id);
}

TEST_F(PeerIdTest, GetBase58) {
  EXPECT_CALL(
      multibase,
      encode(valid_id, libp2p::multi::MultibaseCodec::Encoding::kBase58))
      .WillOnce(Return(std::string{just_string}));

  auto peer_id = createValidPeerId();

  ASSERT_EQ(peer_id.toBase58(), just_string);
}

TEST_F(PeerIdTest, GetPublicKeyWhichIsSet) {
  auto peer_id = createValidPeerId();

  ASSERT_TRUE(peer_id.publicKey());
  ASSERT_EQ(*peer_id.publicKey(), *public_key_shp);
}

TEST_F(PeerIdTest, GetPublicKeyWhichIsUnset) {
  auto peer_id = factory.createPeerId(valid_id).getValue();

  ASSERT_FALSE(peer_id.publicKey());
}

TEST_F(PeerIdTest, SetPublicKeySuccess) {
  // another 'copy' of existing pubkey
  auto pubkey_uptr = std::make_unique<PublicKeyMock>();
  EXPECT_CALL(*pubkey_uptr, getBytes())
      .WillRepeatedly(testing::ReturnRef(public_key_shp->getBytes()));
  EXPECT_CALL(*pubkey_uptr, getType())
      .WillRepeatedly(testing::Return(public_key_shp->getType()));

  EXPECT_CALL(*private_key_shp, publicKey())
      .WillOnce(Return(ByMove(std::move(public_key_uptr))))
      .WillOnce(Return(ByMove(std::move(pubkey_uptr))));
  auto peer_id = factory.createPeerId(valid_id).getValue();
  ASSERT_TRUE(peer_id.setPrivateKey(private_key_shp));

  ASSERT_TRUE(peer_id.setPublicKey(public_key_shp));
  ASSERT_EQ(*peer_id.publicKey(), *public_key_shp);
}

TEST_F(PeerIdTest, SetPublicKeyNotDerivableFromPrivate) {
  // another 'copy' of existing pubkey
  auto pubkey_uptr = std::make_unique<PublicKeyMock>();
  EXPECT_CALL(*pubkey_uptr, getBytes())
      .WillRepeatedly(testing::ReturnRef(public_key_shp->getBytes()));
  EXPECT_CALL(*pubkey_uptr, getType())
      .WillRepeatedly(testing::Return(public_key_shp->getType()));

  EXPECT_CALL(*private_key_shp, publicKey())
      .WillOnce(Return(ByMove(std::move(public_key_uptr))))
      .WillOnce(Return(ByMove(std::move(pubkey_uptr))));
  auto peer_id = factory.createPeerId(valid_id).getValue();
  ASSERT_TRUE(peer_id.setPrivateKey(private_key_shp));

  ASSERT_TRUE(peer_id.setPublicKey(public_key_shp));
  ASSERT_EQ(*peer_id.publicKey(), *public_key_shp);
}

TEST_F(PeerIdTest, GetPrivateKeyWhichIsSet) {}

TEST_F(PeerIdTest, GetPrivateKeyWhichIsUnset) {}

TEST_F(PeerIdTest, SetPrivateKeySuccess) {}

TEST_F(PeerIdTest, SetPrivateKeyNotSourceOfPublic) {}

TEST_F(PeerIdTest, MarshalPublicKeySuccess) {}

TEST_F(PeerIdTest, MarshalPublicKeyFailure) {}

TEST_F(PeerIdTest, MarshalPrivateKeySuccess) {}

TEST_F(PeerIdTest, MarshalPrivateKeyFailure) {}

TEST_F(PeerIdTest, ToString) {}

TEST_F(PeerIdTest, Equals) {}
