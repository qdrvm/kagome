/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/peer_identity.hpp"

#include <gtest/gtest.h>
#include <testutil/outcome.hpp>
#include "libp2p/multi/multibase_codec/multibase_codec_impl.hpp"
#include "libp2p/peer/peer_id.hpp"

using namespace libp2p::multi;
using namespace libp2p::peer;
using namespace kagome::common;

class PeerIdentityTest : public ::testing::Test {
 public:
  std::shared_ptr<MultibaseCodec> codec_ =
      std::make_shared<MultibaseCodecImpl>();

  const Multihash kDefaultMultihash =
      Multihash::create(HashType::sha256,
                        Buffer{}.put("af85e416fa66390b3c834cb6b7aeafb8b4b484e72"
                                     "45fd9a9d81e7f3f5f95714f"))
          .value();

  const PeerId kDefaultPeerId = PeerId::fromHash(kDefaultMultihash).value();

  const std::string kEncodedDefaultPeerId = kDefaultPeerId.toBase58();

  const Multiaddress kDefaultAddress =
      Multiaddress::create("/ip4/192.168.0.1/tcp/228").value();

  const std::string kIdentityString =
      std::string{kDefaultAddress.getStringAddress()} + "/id/"
      + kEncodedDefaultPeerId;
};

/**
 * @given well-formed peer identity string
 * @when creating a PeerIdentity from it
 * @then creation is successful
 */
TEST_F(PeerIdentityTest, FromStringSuccess) {
  EXPECT_OUTCOME_TRUE(identity, PeerIdentity::create(kIdentityString))
  EXPECT_EQ(identity.getIdentity(), kIdentityString);
  EXPECT_EQ(identity.getId(), kDefaultPeerId);
  EXPECT_EQ(identity.getAddress(), kDefaultAddress);
}

/**
 * @given peer identity string without peer's id
 * @when creating a PeerIdentity from it
 * @then creation fails
 */
TEST_F(PeerIdentityTest, FromStringNoId) {
  EXPECT_FALSE(PeerIdentity::create(kDefaultAddress.getStringAddress()));
}

/**
 * @given peer identity string with an ill-formed multiaddress
 * @when creating a PeerIdentity from it
 * @then creation fails
 */
TEST_F(PeerIdentityTest, FromStringIllFormedAddress) {
  EXPECT_FALSE(PeerIdentity::create("/192.168.0.1/id/something"));
}

/**
 * @given peer identity string with id, which is not base58-encoded
 * @when creating a PeerIdentity from it
 * @then creation fails
 */
TEST_F(PeerIdentityTest, FromStringIdNotB58) {
  EXPECT_FALSE(PeerIdentity::create(
      std::string{kDefaultAddress.getStringAddress()} + "/id/something"));
}

/**
 * @given peer identity string with base58-encoded id, which is not sha256
 * multihash
 * @when creating a PeerIdentity from it
 * @then creation is successful
 */
TEST_F(PeerIdentityTest, FromStringIdNotSha256) {
  auto some_str_b58 =
      codec_->encode(Buffer{0x11, 0x22}, MultibaseCodec::Encoding::BASE58);
  EXPECT_FALSE(PeerIdentity::create(
      std::string{kDefaultAddress.getStringAddress()} + "/id/" + some_str_b58));
}

/**
 * @given well-formed peer info structure
 * @when creating a PeerIdentity from it
 * @then creation is successful
 */
TEST_F(PeerIdentityTest, FromInfoSuccess) {
  EXPECT_OUTCOME_TRUE(
      identity,
      PeerIdentity::create(PeerInfo{kDefaultPeerId, {kDefaultAddress}}))
  EXPECT_EQ(identity.getIdentity(), kIdentityString);
  EXPECT_EQ(identity.getId(), kDefaultPeerId);
  EXPECT_EQ(identity.getAddress(), kDefaultAddress);
}

/**
 * @given peer info structure without any multiaddresses
 * @when creating a PeerIdentity from it
 * @then creation is successful
 */
TEST_F(PeerIdentityTest, FromInfoNoAddresses) {
  EXPECT_FALSE(PeerIdentity::create(PeerInfo{kDefaultPeerId, {}}));
}

/**
 * @given PeerId @and Multiaddress structures
 * @when creating a PeerIdentity from them
 * @then creation is successful
 */
TEST_F(PeerIdentityTest, FromDistinctSuccess) {
  EXPECT_OUTCOME_TRUE(identity,
                      PeerIdentity::create(kDefaultPeerId, kDefaultAddress))
  EXPECT_EQ(identity.getIdentity(), kIdentityString);
  EXPECT_EQ(identity.getId(), kDefaultPeerId);
  EXPECT_EQ(identity.getAddress(), kDefaultAddress);
}
