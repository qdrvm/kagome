/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include "common/result.hpp"
#include "core/libp2p/peer/peer_id_test_fixture.hpp"
#include "libp2p/peer/peer_id_factory.hpp"

using namespace libp2p::crypto;
using namespace libp2p::peer;
using namespace libp2p::multi;
using namespace kagome::common;
using namespace kagome::expected;

using testing::ByMove;
using testing::Return;
using testing::ReturnRef;

class PeerIdFactoryTest : public PeerIdTestFixture {};

/**
 * @given initialized factory @and valid peer id in bytes
 * @when creating PeerId from the bytes
 * @then creation succeeds
 */
TEST_F(PeerIdFactoryTest, FromBufferSuccess) {
  auto result = factory.createPeerId(valid_id);

  ASSERT_TRUE(result);
  const auto &peer_id = result.value();
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

  ASSERT_FALSE(result);
}

/**
 * @given initialized factory @and valid peer id @and public key @and private
 * key
 * @when creating PeerId from that triple
 * @then creation succeeds
 */
TEST_F(PeerIdFactoryTest, FromBufferKeysSuccess) {
  setUpValid();
  // make public key a derivative of the private one
  EXPECT_CALL(*private_key_shp, publicKey())
      .WillRepeatedly(Return(ByMove(std::move(public_key_uptr))));

  auto result = factory.createPeerId(valid_id, public_key_shp, private_key_shp);

  ASSERT_TRUE(result);
  const auto &peer_id = result.value();
  ASSERT_EQ(peer_id.toBytes(), valid_id);
  ASSERT_EQ(*peer_id.publicKey(), *public_key_shp);
  ASSERT_EQ(*peer_id.privateKey(), *private_key_shp);
}

/**
 * @given initialized factory @and empty peer id
 * @when creating PeerId from that id
 * @then creation fails
 */
TEST_F(PeerIdFactoryTest, FromBufferKeysEmptyBuffer) {
  auto result = factory.createPeerId(Buffer{}, public_key_shp, private_key_shp);

  ASSERT_FALSE(result);
}

/**
 * @given initialized factory @and valid peer id @and private key @and public
 * key, which is not derived from the private one
 * @when creating PeerId from that triple
 * @then creation fails
 */
TEST_F(PeerIdFactoryTest, FromBufferKeysWrongKeys) {
  EXPECT_CALL(*public_key_shp, getBytes())
      .WillRepeatedly(ReturnRef(just_buffer1));
  EXPECT_CALL(*public_key_uptr, getBytes())
      .WillRepeatedly(ReturnRef(just_buffer2));
  EXPECT_CALL(*private_key_shp, publicKey())
      .WillRepeatedly(Return(ByMove(std::move(public_key_uptr))));

  auto result = factory.createPeerId(valid_id, public_key_shp, private_key_shp);

  ASSERT_FALSE(result);
}

/**
 * @given initialized factory @and public key object
 * @when creating PeerId from that key
 * @then creation succeeds
 */
TEST_F(PeerIdFactoryTest, FromPubkeyObjectSuccess) {
  setUpValid();
  // make public key a derivative of the private one
  EXPECT_CALL(*private_key_shp, publicKey())
      .WillRepeatedly(Return(ByMove(std::move(public_key_uptr))));

  auto result = factory.createFromPublicKey(public_key_shp);

  ASSERT_TRUE(result);
  const auto &peer_id = result.value();
  ASSERT_EQ(peer_id.toBytes(), valid_id);
  ASSERT_EQ(*peer_id.publicKey(), *public_key_shp);
  ASSERT_FALSE(peer_id.privateKey());
}

/**
 * @given initialized factory @and private key object
 * @when creating PeerId from that key
 * @then creation succeeds
 */
TEST_F(PeerIdFactoryTest, FromPrivkeyObjectSuccess) {
  setUpValid();
  // make public key a derivative of the private one
  EXPECT_CALL(*private_key_shp, publicKey())
      .WillRepeatedly(Return(ByMove(std::move(public_key_uptr))));

  auto result = factory.createFromPrivateKey(private_key_shp);

  ASSERT_TRUE(result);
  const auto &peer_id = result.value();
  ASSERT_EQ(peer_id.toBytes(), valid_id);
  ASSERT_EQ(*peer_id.publicKey(), *public_key_shp);
  ASSERT_EQ(*peer_id.privateKey(), *private_key_shp);
}

/**
 * @given initialized factory @and public key bytes
 * @when creating PeerId from those bytes
 * @then creation succeeds
 */
TEST_F(PeerIdFactoryTest, FromPubkeyBufferSuccess) {
  setUpValid();
  EXPECT_CALL(crypto, unmarshalPublicKey(just_buffer1))
      .WillOnce(Return(ByMove(std::move(public_key_uptr))));

  auto result = factory.createFromPublicKey(public_key_shp->getBytes());

  ASSERT_TRUE(result);
  const auto &peer_id = result.value();
  ASSERT_EQ(peer_id.toBytes(), valid_id);
  ASSERT_EQ(*peer_id.publicKey(), *public_key_shp);
  ASSERT_FALSE(peer_id.privateKey());
}

/**
 * @given initialized factory @and invalid public key bytes
 * @when creating PeerId from that key
 * @then creation succeeds
 */
TEST_F(PeerIdFactoryTest, FromPubkeyBufferWrongKey) {
  setUpValid();
  EXPECT_CALL(crypto, unmarshalPublicKey(just_buffer1))
      .WillOnce(Return(ByMove(nullptr)));

  auto result = factory.createFromPublicKey(public_key_shp->getBytes());

  ASSERT_FALSE(result);
}

/**
 * @given initialized factory @and private key bytes
 * @when creating PeerId from those bytes
 * @then creation succeeds
 */
TEST_F(PeerIdFactoryTest, FromPrivkeyBufferSuccess) {
  setUpValid();
  EXPECT_CALL(*private_key_uptr, publicKey())
      .WillRepeatedly(Return(ByMove(std::move(public_key_uptr))));
  EXPECT_CALL(crypto, unmarshalPrivateKey(just_buffer2))
      .WillOnce(Return(ByMove(std::move(private_key_uptr))));

  auto result = factory.createFromPrivateKey(private_key_shp->getBytes());

  ASSERT_TRUE(result);
  const auto &peer_id = result.value();
  ASSERT_EQ(peer_id.toBytes(), valid_id);
  ASSERT_EQ(*peer_id.publicKey(), *public_key_shp);
  ASSERT_EQ(*peer_id.privateKey(), *private_key_shp);
}

/**
 * @given initialized factory @and invalid private key bytes
 * @when creating PeerId from that key
 * @then creation succeeds
 */
TEST_F(PeerIdFactoryTest, FromPrivkeyBufferWrongKey) {
  setUpValid();
  EXPECT_CALL(crypto, unmarshalPrivateKey(just_buffer2))
      .WillOnce(Return(ByMove(nullptr)));

  auto result = factory.createFromPrivateKey(private_key_shp->getBytes());

  ASSERT_FALSE(result);
}

TEST_F(PeerIdFactoryTest, FromPubkeyStringSuccess) {
  setUpValid();
  EXPECT_CALL(multibase, decode(std::string_view{just_string}))
      .WillOnce(Return(Value{just_buffer1}));
  EXPECT_CALL(crypto, unmarshalPublicKey(just_buffer1))
      .WillOnce(Return(ByMove(std::move(public_key_uptr))));

  auto result = factory.createFromPublicKey(just_string);

  ASSERT_TRUE(result);
  const auto &peer_id = result.value();
  ASSERT_EQ(peer_id.toBytes(), valid_id);
  ASSERT_EQ(*peer_id.publicKey(), *public_key_shp);
  ASSERT_FALSE(peer_id.privateKey());
}

TEST_F(PeerIdFactoryTest, FromPubkeyStringWrongKey) {
  EXPECT_CALL(multibase, decode(std::string_view{just_string}))
      .WillOnce(Return(Error{"foo"}));

  auto result = factory.createFromPublicKey(just_string);

  ASSERT_FALSE(result);
}

TEST_F(PeerIdFactoryTest, FromPrivkeyStringSuccess) {
  setUpValid();
  EXPECT_CALL(multibase, decode(std::string_view{just_string}))
      .WillOnce(Return(Value{just_buffer2}));
  EXPECT_CALL(*private_key_uptr, publicKey())
      .WillRepeatedly(Return(ByMove(std::move(public_key_uptr))));
  EXPECT_CALL(crypto, unmarshalPrivateKey(just_buffer2))
      .WillOnce(Return(ByMove(std::move(private_key_uptr))));

  auto result = factory.createFromPrivateKey(just_string);

  ASSERT_TRUE(result);
  const auto &peer_id = result.value();
  ASSERT_EQ(peer_id.toBytes(), valid_id);
  ASSERT_EQ(*peer_id.publicKey(), *public_key_shp);
  ASSERT_EQ(*peer_id.privateKey(), *private_key_shp);
}

TEST_F(PeerIdFactoryTest, FromPrivkeyStringWrongKey) {
  EXPECT_CALL(multibase, decode(std::string_view{just_string}))
      .WillOnce(Return(Error{"foo"}));

  auto result = factory.createFromPrivateKey(just_string);

  ASSERT_FALSE(result);
}

TEST_F(PeerIdFactoryTest, FromEncodedStringSuccess) {
  EXPECT_CALL(multibase, decode(std::string_view{just_string}))
      .WillOnce(Return(Value{valid_id}));

  auto result = factory.createFromEncodedString(just_string);

  ASSERT_TRUE(result);
  const auto &peer_id = result.value();
  ASSERT_EQ(peer_id.toBytes(), valid_id);
  ASSERT_FALSE(peer_id.publicKey());
  ASSERT_FALSE(peer_id.privateKey());
}

TEST_F(PeerIdFactoryTest, FromEncodedBadEncoding) {
  EXPECT_CALL(multibase, decode(std::string_view{just_string}))
      .WillOnce(Return(Error{"foo"}));

  auto result = factory.createFromEncodedString(just_string);

  ASSERT_FALSE(result);
}
