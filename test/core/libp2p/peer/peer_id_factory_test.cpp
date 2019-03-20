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

  /// any non-empty sequence of bytes is a valid id
  const Buffer valid_id{0x54, 0x55, 0x56};
};

TEST_F(PeerIdFactoryTest, FromBufferSuccess) {}

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
