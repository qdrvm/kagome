/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "libp2p/security/plaintext/exchange_message_marshaller_impl.hpp"
#include "libp2p/crypto/key.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "mock/libp2p/crypto/key_marshaller_mock.hpp"
#include "testutil/outcome.hpp"

using libp2p::security::plaintext::ExchangeMessageMarshallerImpl;
using libp2p::security::plaintext::ExchangeMessageMarshaller;
using libp2p::security::plaintext::ExchangeMessage;
using libp2p::crypto::PublicKey;
using libp2p::peer::PeerId;

TEST(ExchangeMessageMarshallerTest, Create) {
  std::shared_ptr<ExchangeMessageMarshaller> marshaller = std::make_shared<ExchangeMessageMarshallerImpl>();
  PublicKey pk {
    .type = libp2p::crypto::Key::Type::ED25519,
    .data = std::vector<uint8_t>(255, 1)
  };
  auto pid = PeerId::fromPublicKey(pk);
  ExchangeMessage msg {.pubkey = pk, .peer_id = pid};
  EXPECT_OUTCOME_TRUE(bytes, marshaller->marshal(msg));
  ASSERT_EQ(bytes, marshaller->unmarshal(bytes));
}