/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/identity/peer_identity_factory_impl.hpp"

#include <gtest/gtest.h>
#include <testutil/outcome.hpp>
#include "libp2p/multi/multibase_codec/multibase_codec_impl.hpp"

using namespace libp2p::multi;
using namespace libp2p::peer;
using namespace kagome::common;

class PeerIdentityFactoryTest : public ::testing::Test {
 public:
  std::shared_ptr<MultibaseCodec> codec_ =
      std::make_shared<MultibaseCodecImpl>();
  std::shared_ptr<PeerIdentityFactory> factory_ =
      std::make_shared<PeerIdentityFactoryImpl>(codec_);

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

TEST_F(PeerIdentityFactoryTest, FromStringSuccess) {
  EXPECT_OUTCOME_TRUE(identity, factory_->create(kIdentityString))
  EXPECT_EQ(identity.getIdentity(), kIdentityString);
  EXPECT_EQ(identity.getId(), kDefaultPeerId);
  EXPECT_EQ(identity.getAddress(), kDefaultAddress);
}

TEST_F(PeerIdentityFactoryTest, FromStringNoId) {
  EXPECT_FALSE(factory_->create(kDefaultAddress.getStringAddress()));
}

TEST_F(PeerIdentityFactoryTest, FromStringIllFormedAddress) {
  EXPECT_FALSE(factory_->create("/192.168.0.1/id/something"));
}

TEST_F(PeerIdentityFactoryTest, FromStringIdNotB58) {
  EXPECT_FALSE(factory_->create(std::string{kDefaultAddress.getStringAddress()}
                                + "/id/something"));
}

TEST_F(PeerIdentityFactoryTest, FromStringIdNotSha256) {
  auto some_str_b58 =
      codec_->encode(Buffer{0x11, 0x22}, MultibaseCodec::Encoding::BASE58);
  EXPECT_FALSE(factory_->create(std::string{kDefaultAddress.getStringAddress()}
                                + "/id/" + some_str_b58));
}

TEST_F(PeerIdentityFactoryTest, FromInfoSuccess) {
  EXPECT_OUTCOME_TRUE(
      identity, factory_->create(PeerInfo{kDefaultPeerId, {kDefaultAddress}}))
  EXPECT_EQ(identity.getIdentity(), kIdentityString);
  EXPECT_EQ(identity.getId(), kDefaultPeerId);
  EXPECT_EQ(identity.getAddress(), kDefaultAddress);
}

TEST_F(PeerIdentityFactoryTest, FromInfoIdNotSha256) {
  EXPECT_OUTCOME_TRUE(
      not_sha256_hash,
      Multihash::create(HashType::sha512,
                        Buffer{}.put("af85e416fa66390b3c834cb6b7aeafb8b4b484e72"
                                     "45fd9a9d81e7f3f5f95714f")))

  EXPECT_FALSE(factory_->create(PeerInfo{not_sha256_hash, {kDefaultAddress}}));
}

TEST_F(PeerIdentityFactoryTest, FromInfoNoAddresses) {
  EXPECT_FALSE(factory_->create(PeerInfo{kDefaultPeerId, {}}));
}

TEST_F(PeerIdentityFactoryTest, FromDistinctSuccess) {
  EXPECT_OUTCOME_TRUE(identity,
                      factory_->create(kDefaultPeerId, kDefaultAddress))
  EXPECT_EQ(identity.getIdentity(), kIdentityString);
  EXPECT_EQ(identity.getId(), kDefaultPeerId);
  EXPECT_EQ(identity.getAddress(), kDefaultAddress);
}

TEST_F(PeerIdentityFactoryTest, FromDistinctIdNotSha256) {
  EXPECT_OUTCOME_TRUE(
      not_sha256_hash,
      Multihash::create(HashType::sha512,
                        Buffer{}.put("af85e416fa66390b3c834cb6b7aeafb8b4b484e72"
                                     "45fd9a9d81e7f3f5f95714f")))

  EXPECT_FALSE(factory_->create(not_sha256_hash, kDefaultAddress));
}
