/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/security/plaintext/plaintext.hpp"

#include <gtest/gtest.h>
#include <testutil/gmock_actions.hpp>
#include <testutil/outcome.hpp>
#include "common/buffer.hpp"
#include "libp2p/multi/multihash.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "mock/libp2p/connection/raw_connection_mock.hpp"

using namespace libp2p::connection;
using namespace libp2p::security;
using namespace libp2p::multi;

using kagome::common::Buffer;
using libp2p::peer::PeerId;
using testing::Return;

class PlaintextAdaptorTest : public testing::Test {
 public:
  std::shared_ptr<SecurityAdaptor> adaptor_ = std::make_shared<Plaintext>();

  std::shared_ptr<RawConnectionMock> connection_ =
      std::make_shared<RawConnectionMock>();

  const PeerId kDefaultPeerId =
      PeerId::fromHash(
          Multihash::create(HashType::sha256, Buffer{0x11, 0x22}).value())
          .value();
};

/**
 * @given plaintext security adaptor
 * @when getting id of the underlying security protocol
 * @then an expected id is returned
 */
TEST_F(PlaintextAdaptorTest, GetId) {
  ASSERT_EQ(adaptor_->getProtocolId(), "/plaintext/1.0.0");
}

/**
 * @given plaintext security adaptor
 * @when securing a raw connection inbound, using that adaptor
 * @then connection is secured
 */
TEST_F(PlaintextAdaptorTest, SecureInbound) {
  EXPECT_CALL(*connection_, isClosed()).WillOnce(Return(false));

  adaptor_->secureInbound(connection_, [](auto &&conn_res) {
    EXPECT_OUTCOME_TRUE(sec_conn, conn_res)
    ASSERT_FALSE(sec_conn->isClosed());
  });
}

/**
 * @given plaintext security adaptor
 * @when securing a raw connection outbound, using that adaptor
 * @then connection is secured
 */
TEST_F(PlaintextAdaptorTest, SecureOutbound) {
  EXPECT_CALL(*connection_, isClosed()).WillOnce(Return(false));

  adaptor_->secureOutbound(connection_, kDefaultPeerId, [](auto &&conn_res) {
    EXPECT_OUTCOME_TRUE(sec_conn, conn_res)
    ASSERT_FALSE(sec_conn->isClosed());
  });
}
