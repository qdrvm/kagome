/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/connection/plaintext/plaintext.hpp"

#include <gtest/gtest.h>
#include <testutil/outcome.hpp>
#include "mock/libp2p/connection/raw_connection_mock.hpp"
#include "testutil/literals.hpp"

using namespace libp2p::connection;

using testing::ByMove;
using testing::Return;

class PlaintextConnectionTest : public testing::Test {
 public:
  std::shared_ptr<RawConnectionMock> connection_ =
      std::make_shared<RawConnectionMock>();

  std::shared_ptr<SecureConnection> secure_connection_ =
      std::make_shared<PlaintextConnection>(connection_);

  std::vector<uint8_t> bytes_{0x11, 0x22};
};

/**
 * @given plaintext secure connection
 * @when invoking localPeer method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, LocalPeer) {
  ASSERT_FALSE(secure_connection_->localPeer());
}

/**
 * @given plaintext secure connection
 * @when invoking remotePeer method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, RemotePeer) {
  ASSERT_FALSE(secure_connection_->remotePeer());
}

/**
 * @given plaintext secure connection
 * @when invoking remotePublicKey method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, RemotePublicKey) {
  ASSERT_FALSE(secure_connection_->remotePublicKey());
}

/**
 * @given plaintext secure connection
 * @when invoking isInitiator method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, IsInitiator) {
  EXPECT_CALL(*connection_, isInitiator_hack()).WillOnce(Return(true));
  ASSERT_TRUE(secure_connection_->isInitiator());
}

/**
 * @given plaintext secure connection
 * @when invoking localMultiaddr method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, LocalMultiaddr) {
  const auto kDefaultMultiaddr = "/ip4/192.168.0.1/tcp/226"_multiaddr;

  EXPECT_CALL(*connection_, localMultiaddr())
      .WillOnce(Return(kDefaultMultiaddr));
  EXPECT_OUTCOME_TRUE(ma, secure_connection_->localMultiaddr())
  ASSERT_EQ(ma.getStringAddress(), kDefaultMultiaddr.getStringAddress());
}

/**
 * @given plaintext secure connection
 * @when invoking remoteMultiaddr method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, RemoteMultiaddr) {
  const auto kDefaultMultiaddr = "/ip4/192.168.0.1/tcp/226"_multiaddr;

  EXPECT_CALL(*connection_, remoteMultiaddr())
      .WillOnce(Return(kDefaultMultiaddr));
  EXPECT_OUTCOME_TRUE(ma, secure_connection_->remoteMultiaddr())
  ASSERT_EQ(ma.getStringAddress(), kDefaultMultiaddr.getStringAddress());
}

/**
 * @given plaintext secure connection
 * @when invoking read method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, Read) {
  EXPECT_CALL(*connection_, read(1)).WillOnce(Return(bytes_));
  EXPECT_OUTCOME_TRUE(read_bytes, secure_connection_->read(1))
  ASSERT_EQ(read_bytes, bytes_);
}

/**
 * @given plaintext secure connection
 * @when invoking readSome method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, ReadSome) {
  EXPECT_CALL(*connection_, readSome(1)).WillOnce(Return(bytes_));
  EXPECT_OUTCOME_TRUE(read_bytes, secure_connection_->readSome(1))
  ASSERT_EQ(read_bytes, bytes_);
}

/**
 * @given plaintext secure connection
 * @when invoking read method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, ReadSpan) {
  EXPECT_CALL(*connection_, read(gsl::span<uint8_t>(bytes_)))
      .WillOnce(Return(1));
  EXPECT_OUTCOME_TRUE(bytes_read, secure_connection_->read(bytes_))
  ASSERT_EQ(bytes_read, 1);
}

/**
 * @given plaintext secure connection
 * @when invoking readSome method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, ReadSomeSpan) {
  EXPECT_CALL(*connection_, readSome(gsl::span<uint8_t>(bytes_)))
      .WillOnce(Return(1));
  EXPECT_OUTCOME_TRUE(bytes_read, secure_connection_->readSome(bytes_))
  ASSERT_EQ(bytes_read, 1);
}

/**
 * @given plaintext secure connection
 * @when invoking write method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, Write) {
  EXPECT_CALL(*connection_, write(gsl::span<const uint8_t>(bytes_)))
      .WillOnce(Return(1));
  EXPECT_OUTCOME_TRUE(bytes_written, secure_connection_->write(bytes_))
  ASSERT_EQ(bytes_written, 1);
}

/**
 * @given plaintext secure connection
 * @when invoking writeSome method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, WriteSome) {
  EXPECT_CALL(*connection_, writeSome(gsl::span<const uint8_t>(bytes_)))
      .WillOnce(Return(1));
  EXPECT_OUTCOME_TRUE(bytes_written, secure_connection_->writeSome(bytes_))
  ASSERT_EQ(bytes_written, 1);
}

/**
 * @given plaintext secure connection
 * @when invoking isClosed method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, IsClosed) {
  EXPECT_CALL(*connection_, isClosed()).WillOnce(Return(false));
  ASSERT_FALSE(secure_connection_->isClosed());
}

/**
 * @given plaintext secure connection
 * @when invoking close method of the connection
 * @then method behaves as expected
 */
TEST_F(PlaintextConnectionTest, Close) {
  EXPECT_CALL(*connection_, close()).WillOnce(Return(outcome::success()));
  ASSERT_TRUE(secure_connection_->close());
}
