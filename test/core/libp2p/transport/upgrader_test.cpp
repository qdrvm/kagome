/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/impl/upgrader_impl.hpp"

#include <numeric>
#include <unordered_map>

#include <gtest/gtest.h>
#include <testutil/outcome.hpp>
#include "libp2p/multi/multihash.hpp"
#include "mock/libp2p/connection/raw_connection_mock.hpp"
#include "mock/libp2p/connection/secure_connection_mock.hpp"
#include "mock/libp2p/muxer/muxer_adaptor_mock.hpp"
#include "mock/libp2p/protocol_muxer/protocol_muxer_mock.hpp"
#include "mock/libp2p/security/security_adaptor_mock.hpp"

using namespace libp2p::transport;
using namespace libp2p::muxer;
using namespace libp2p::security;
using namespace libp2p::peer;
using namespace libp2p::connection;
using namespace libp2p::protocol_muxer;
using namespace libp2p::basic;

using testing::Return;

class UpgraderTest : public testing::Test {
 protected:
  void SetUp() override {
    for (size_t i = 0; i < security_protos_.size(); ++i) {
      ON_CALL(
          *std::static_pointer_cast<SecurityAdaptorMock>(security_mocks_[i]),
          getProtocolId())
          .WillByDefault(Return(security_protos_[i]));
    }
    //    for (size_t i = 0; i < muxer_protos_.size(); ++i) {
    //      ON_CALL(*muxer_mocks_[i], getProtocolId())
    //          .WillByDefault(Return(muxer_protos_[i]));
    //    }

    upgrader_ = std::make_shared<UpgraderImpl>(peer_id_, multiselect_mock_,
                                               security_mocks_, muxer_mocks_);
  }

  PeerId peer_id_ = PeerId::fromHash(libp2p::multi::Multihash::create(
                                         libp2p::multi::HashType::sha256,
                                         kagome::common::Buffer{0x11, 0x22})
                                         .value())
                        .value();

  std::shared_ptr<ProtocolMuxerMock> multiselect_mock_ =
      std::make_shared<ProtocolMuxerMock>();

  std::vector<Protocol> security_protos_{"security_proto1", "security_proto2"};
  std::vector<std::shared_ptr<SecurityAdaptor>> security_mocks_{
      std::make_shared<SecurityAdaptorMock>(),
      std::make_shared<SecurityAdaptorMock>()};

  std::vector<Protocol> muxer_protos_{"muxer_proto1", "muxer_proto2"};
  std::vector<std::shared_ptr<MuxerAdaptor>> muxer_mocks_{
      std::make_shared<MuxerAdaptorMock>(),
      std::make_shared<MuxerAdaptorMock>()};

  std::shared_ptr<Upgrader> upgrader_;

  std::shared_ptr<RawConnectionMock> raw_conn_ =
      std::make_shared<RawConnectionMock>();
  std::shared_ptr<SecureConnectionMock> sec_conn_ =
      std::make_shared<SecureConnectionMock>();
};

TEST_F(UpgraderTest, UpgradeSecureInitiator) {
  EXPECT_CALL(*raw_conn_, isInitiator_hack()).WillOnce(Return(true));
  EXPECT_CALL(
      *multiselect_mock_,
      selectOneOf(gsl::span<const Protocol>(security_protos_),
                  std::static_pointer_cast<ReadWriteCloser>(raw_conn_), true))
      .WillOnce(Return(security_protos_[0]));
  EXPECT_CALL(
      *std::static_pointer_cast<SecurityAdaptorMock>(security_mocks_[0]),
      secureOutbound(std::static_pointer_cast<RawConnection>(raw_conn_),
                     peer_id_))
      .WillOnce(Return(sec_conn_));

  EXPECT_OUTCOME_TRUE(upgraded_conn, upgrader_->upgradeToSecure(raw_conn_))
  ASSERT_EQ(upgraded_conn, sec_conn_);
}

TEST_F(UpgraderTest, UpgradeSecureNotInitiator) {
  EXPECT_CALL(*raw_conn_, isInitiator_hack()).WillOnce(Return(false));
  EXPECT_CALL(
      *multiselect_mock_,
      selectOneOf(gsl::span<const Protocol>(security_protos_),
                  std::static_pointer_cast<ReadWriteCloser>(raw_conn_), false))
      .WillOnce(Return(outcome::success(security_protos_[1])));
  EXPECT_CALL(
      *std::static_pointer_cast<SecurityAdaptorMock>(security_mocks_[1]),
      secureInbound(std::static_pointer_cast<RawConnection>(raw_conn_)))
      .WillOnce(Return(outcome::success(sec_conn_)));

  EXPECT_OUTCOME_TRUE(upgraded_conn, upgrader_->upgradeToSecure(raw_conn_))
  ASSERT_EQ(upgraded_conn, sec_conn_);
}

TEST_F(UpgraderTest, UpgradeSecureFail) {
  EXPECT_CALL(*raw_conn_, isInitiator_hack()).WillOnce(Return(false));
  EXPECT_CALL(
      *multiselect_mock_,
      selectOneOf(gsl::span<const Protocol>(security_protos_),
                  std::static_pointer_cast<ReadWriteCloser>(raw_conn_), false))
      .WillOnce(Return(outcome::failure(std::error_code())));

  ASSERT_FALSE(upgrader_->upgradeToSecure(raw_conn_));
}

TEST_F(UpgraderTest, UpgradeMux) {}

TEST_F(UpgraderTest, UpgradeMuxFail) {}
