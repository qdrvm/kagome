/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/peer_id_factory.hpp"
#include <gtest/gtest.h>
#include "core/libp2p/crypto/crypto_provider_mock.hpp"
#include "core/libp2p/multi/multibase_codec_mock.hpp"

using namespace libp2p::crypto;
using namespace libp2p::peer;
using namespace libp2p::multi;
using namespace kagome::common;

class PeerIdFactoryTest : public ::testing::Test {
 public:
  const CryptoProviderMock crypto{};
  const MultibaseCodecMock multibase{};
  const PeerIdFactory factory{multibase, crypto};

  /// must be a SHA-256 multihash
  const Buffer valid_id{0x12, 0x02, 0x56, 0x57};
  const Buffer invalid_id{0x66, 0x43};
};

TEST_F(PeerIdFactoryTest, FromBufferSuccess) {
  auto result = factory.createPeerId(valid_id);

  ASSERT_TRUE(result.hasValue());
  const auto &peer_id = result.getValueRef();
  ASSERT_EQ(peer_id.toBytes(), valid_id);
  ASSERT_FALSE(peer_id.publicKey());
  ASSERT_FALSE(peer_id.privateKey());
}

TEST_F(PeerIdFactoryTest, FromBufferWrongBuffer) {}

TEST_F(PeerIdFactoryTest, FromBufferKeysSuccess) {}

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
