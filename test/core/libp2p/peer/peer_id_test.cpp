/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/peer_id.hpp"
#include <gtest/gtest.h>
#include "core/libp2p/peer/peer_id_test_fixture.hpp"

using namespace libp2p::peer;
using namespace libp2p::crypto;
using namespace kagome::common;

using ::testing::ByMove;
using ::testing::Matcher;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;

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
        .value();
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
      .WillOnce(Return(just_string));

  auto peer_id = createValidPeerId();

  ASSERT_EQ(peer_id.toBase58(), just_string);
}

TEST_F(PeerIdTest, GetPublicKeyWhichIsSet) {
  auto peer_id = createValidPeerId();

  ASSERT_TRUE(peer_id.publicKey());
  ASSERT_EQ(*peer_id.publicKey(), *public_key_shp);
}

TEST_F(PeerIdTest, GetPublicKeyWhichIsUnset) {
  auto peer_id_res = factory.createPeerId(valid_id);

  ASSERT_TRUE(peer_id_res);
  ASSERT_FALSE(peer_id_res.value().publicKey());
}

TEST_F(PeerIdTest, SetPublicKeySuccess) {
  // another 'copy' of existing pubkey
  auto pubkey_uptr = std::make_unique<PublicKeyMock>();
  EXPECT_CALL(*pubkey_uptr, getBytes())
      .WillRepeatedly(ReturnRef(public_key_shp->getBytes()));
  EXPECT_CALL(*pubkey_uptr, getType())
      .WillRepeatedly(testing::Return(public_key_shp->getType()));

  EXPECT_CALL(*private_key_shp, publicKey())
      .WillOnce(Return(ByMove(std::move(public_key_uptr))))
      .WillOnce(Return(ByMove(std::move(pubkey_uptr))));
  auto peer_id_res = factory.createPeerId(valid_id);
  ASSERT_TRUE(peer_id_res);
  auto &&peer_id = std::move(peer_id_res.value());
  ASSERT_TRUE(peer_id.setPrivateKey(private_key_shp));

  ASSERT_TRUE(peer_id.setPublicKey(public_key_shp));
  ASSERT_EQ(*peer_id.publicKey(), *public_key_shp);
}

TEST_F(PeerIdTest, SetPublicKeyNotDerivableFromPrivate) {
  // make key, which is not equal to the existing public key
  auto pubkey_uptr = std::make_unique<PublicKeyMock>();
  EXPECT_CALL(*pubkey_uptr, getBytes()).WillRepeatedly(ReturnRef(just_buffer2));
  EXPECT_CALL(*pubkey_uptr, getType())
      .WillRepeatedly(testing::Return(public_key_shp->getType()));

  EXPECT_CALL(*private_key_shp, publicKey())
      .WillOnce(Return(ByMove(std::move(public_key_uptr))))
      .WillOnce(Return(ByMove(std::move(pubkey_uptr))));
  auto peer_id_res = factory.createPeerId(valid_id);
  ASSERT_TRUE(peer_id_res);
  auto &&peer_id = std::move(peer_id_res.value());
  ASSERT_TRUE(peer_id.setPrivateKey(private_key_shp));

  ASSERT_FALSE(peer_id.setPublicKey(public_key_shp));
}

TEST_F(PeerIdTest, GetPrivateKeyWhichIsSet) {
  auto peer_id = createValidPeerId();

  ASSERT_TRUE(peer_id.privateKey());
  ASSERT_EQ(*peer_id.privateKey(), *private_key_shp);
}

TEST_F(PeerIdTest, GetPrivateKeyWhichIsUnset) {
  auto peer_id_res = factory.createPeerId(valid_id);
  ASSERT_TRUE(peer_id_res);
  auto &&peer_id = std::move(peer_id_res.value());

  ASSERT_FALSE(peer_id.privateKey());
}

TEST_F(PeerIdTest, SetPrivateKeySuccess) {
  EXPECT_CALL(*private_key_shp, publicKey())
      .WillOnce(Return(ByMove(std::move(public_key_uptr))));
  auto peer_id_res = factory.createPeerId(valid_id);
  ASSERT_TRUE(peer_id_res);
  auto &&peer_id = std::move(peer_id_res.value());
  ASSERT_TRUE(peer_id.setPublicKey(public_key_shp));

  ASSERT_TRUE(peer_id.setPrivateKey(private_key_shp));
  ASSERT_EQ(*peer_id.privateKey(), *private_key_shp);
}

TEST_F(PeerIdTest, SetPrivateKeyNotSourceOfPublic) {
  // make key, which is not equal to the existing public key
  auto pubkey_uptr = std::make_unique<PublicKeyMock>();
  EXPECT_CALL(*pubkey_uptr, getBytes()).WillRepeatedly(ReturnRef(just_buffer2));
  EXPECT_CALL(*pubkey_uptr, getType())
      .WillRepeatedly(testing::Return(public_key_shp->getType()));

  EXPECT_CALL(*private_key_shp, publicKey())
      .WillOnce(Return(ByMove(std::move(pubkey_uptr))));
  auto peer_id_res = factory.createPeerId(valid_id);
  ASSERT_TRUE(peer_id_res);
  auto &&peer_id = std::move(peer_id_res.value());
  ASSERT_TRUE(peer_id.setPublicKey(public_key_shp));

  ASSERT_FALSE(peer_id.setPrivateKey(private_key_shp));
}

TEST_F(PeerIdTest, MarshalPublicKeySuccess) {
  EXPECT_CALL(
      crypto,
      marshal(testing::Matcher<const PublicKey &>(Ref(*public_key_shp))))
      .WillOnce(Return((just_buffer2)));

  auto peer_id = createValidPeerId();

  auto marshalled_pubkey = peer_id.marshalPublicKey();
  ASSERT_TRUE(marshalled_pubkey);
  ASSERT_EQ(*marshalled_pubkey, just_buffer2);
}

TEST_F(PeerIdTest, MarshalPublicKeyNoKey) {
  auto peer_id_res = factory.createPeerId(valid_id);
  ASSERT_TRUE(peer_id_res);
  auto &&peer_id = std::move(peer_id_res.value());

  ASSERT_FALSE(peer_id.marshalPublicKey());
}

TEST_F(PeerIdTest, MarshalPrivateKeySuccess) {
  EXPECT_CALL(
      crypto,
      marshal(testing::Matcher<const PrivateKey &>(Ref(*private_key_shp))))
      .WillOnce(Return(just_buffer2));

  auto peer_id = createValidPeerId();

  auto marshalled_privkey = peer_id.marshalPrivateKey();
  ASSERT_TRUE(marshalled_privkey);
  ASSERT_EQ(*marshalled_privkey, just_buffer2);
}

TEST_F(PeerIdTest, MarshalPrivateKeyNoKey) {
  auto peer_id_res = factory.createPeerId(valid_id);
  ASSERT_TRUE(peer_id_res);
  auto &&peer_id = std::move(peer_id_res.value());

  ASSERT_FALSE(peer_id.marshalPrivateKey());
}
