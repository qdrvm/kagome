/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/peer_identity.hpp"

#include <gtest/gtest.h>
#include <testutil/outcome.hpp>
#include "libp2p/multi/multibase_codec/multibase_codec_impl.hpp"

using namespace libp2p::multi;
using namespace libp2p::peer;
using namespace kagome::common;

class PeerIdentityTest : public ::testing::Test {
 public:
  std::shared_ptr<MultibaseCodec> codec_ =
      std::make_shared<MultibaseCodecImpl>();

  const PeerId kDefaultPeerId =
      Multihash::create(HashType::sha256,
                        Buffer{}.put("af85e416fa66390b3c834cb6b7aeafb8b4b484e72"
                                     "45fd9a9d81e7f3f5f95714f"))
          .value();
  const std::string kEncodedDefaultPeerId = codec_->encode(
      kDefaultPeerId.toBuffer(), MultibaseCodec::Encoding::BASE58);

  const Multiaddress kDefaultAddress =
      Multiaddress::create("/ip4/192.168.0.1/tcp/228").value();

  const std::string kIdentityString =
      std::string{kDefaultAddress.getStringAddress()} + "/id/"
      + kEncodedDefaultPeerId;
};

TEST_F(PeerIdentityTest, FromStringSuccess) {
  EXPECT_OUTCOME_TRUE(identity, PeerIdentity::create(kIdentityString))
  EXPECT_EQ(identity.getIdentity(), kIdentityString);
  EXPECT_EQ(identity.getId(), kDefaultPeerId);
  EXPECT_EQ(identity.getAddress(), kDefaultAddress);
}

TEST_F(PeerIdentityTest, FromStringNoId) {
  EXPECT_FALSE(PeerIdentity::create(kDefaultAddress.getStringAddress()));
}

TEST_F(PeerIdentityTest, FromStringIllFormedAddress) {
  EXPECT_FALSE(PeerIdentity::create("/192.168.0.1/id/something"));
}

TEST_F(PeerIdentityTest, FromStringIdNotB58) {
  EXPECT_FALSE(PeerIdentity::create(
      std::string{kDefaultAddress.getStringAddress()} + "/id/something"));
}

TEST_F(PeerIdentityTest, FromStringIdNotSha256) {
  auto some_str_b58 =
      codec_->encode(Buffer{0x11, 0x22}, MultibaseCodec::Encoding::BASE58);
  EXPECT_FALSE(PeerIdentity::create(
      std::string{kDefaultAddress.getStringAddress()} + "/id/" + some_str_b58));
}

TEST_F(PeerIdentityTest, FromInfoSuccess) {
  EXPECT_OUTCOME_TRUE(
      identity,
      PeerIdentity::create(PeerInfo{kDefaultPeerId, {kDefaultAddress}}))
  EXPECT_EQ(identity.getIdentity(), kIdentityString);
  EXPECT_EQ(identity.getId(), kDefaultPeerId);
  EXPECT_EQ(identity.getAddress(), kDefaultAddress);
}

TEST_F(PeerIdentityTest, FromInfoIdNotSha256) {
  EXPECT_OUTCOME_TRUE(
      not_sha256_hash,
      Multihash::create(HashType::sha512,
                        Buffer{}.put("af85e416fa66390b3c834cb6b7aeafb8b4b484e72"
                                     "45fd9a9d81e7f3f5f95714f")))

  EXPECT_FALSE(
      PeerIdentity::create(PeerInfo{not_sha256_hash, {kDefaultAddress}}));
}

TEST_F(PeerIdentityTest, FromInfoNoAddresses) {
  EXPECT_FALSE(PeerIdentity::create(PeerInfo{kDefaultPeerId, {}}));
}

TEST_F(PeerIdentityTest, FromDistinctSuccess) {
  EXPECT_OUTCOME_TRUE(identity,
                      PeerIdentity::create(kDefaultPeerId, kDefaultAddress))
  EXPECT_EQ(identity.getIdentity(), kIdentityString);
  EXPECT_EQ(identity.getId(), kDefaultPeerId);
  EXPECT_EQ(identity.getAddress(), kDefaultAddress);
}

TEST_F(PeerIdentityTest, FromDistinctIdNotSha256) {
  EXPECT_OUTCOME_TRUE(
      not_sha256_hash,
      Multihash::create(HashType::sha512,
                        Buffer{}.put("af85e416fa66390b3c834cb6b7aeafb8b4b484e72"
                                     "45fd9a9d81e7f3f5f95714f")))

  EXPECT_FALSE(PeerIdentity::create(not_sha256_hash, kDefaultAddress));
}
