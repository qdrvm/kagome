/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/security/plaintext/plaintext.hpp"

#include <gtest/gtest.h>
#include <boost/di.hpp>
#include <boost/di/extension/providers/mocks_provider.hpp>
#include <testutil/gmock_actions.hpp>
#include <testutil/outcome.hpp>
#include "common/buffer.hpp"
#include "libp2p/crypto/key_marshaller.hpp"
#include "libp2p/multi/multihash.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "libp2p/transport/tcp.hpp"
#include "mock/libp2p/connection/raw_connection_mock.hpp"
#include "mock/libp2p/crypto/key_marshaller_mock.hpp"
#include "mock/libp2p/peer/identity_manager_mock.hpp"
#include "testutil/literals.hpp"

using namespace libp2p;
using namespace connection;
using namespace security;
using namespace peer;
using namespace crypto;
using namespace transport;
using namespace marshaller;

using libp2p::peer::PeerId;
using testing::_;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;

class PlaintextAdaptorTest : public testing::Test {
 public:
  std::shared_ptr<NiceMock<IdentityManagerMock>> idmgr =
      std::make_shared<NiceMock<IdentityManagerMock>>();

  std::shared_ptr<NiceMock<KeyMarshallerMock>> marshaller =
      std::make_shared<NiceMock<KeyMarshallerMock>>();

  std::shared_ptr<Plaintext> adaptor =
      std::make_shared<Plaintext>(marshaller, idmgr);

  void SetUp() override {
    ON_CALL(*conn, readSome(_, _, _)).WillByDefault(Arg2CallbackWithArg(5));
    ON_CALL(*conn, write(_, _, _)).WillByDefault(Arg2CallbackWithArg(5));

    ON_CALL(*marshaller, unmarshalPublicKey(_))
        .WillByDefault(Return(publicKey));

    ON_CALL(*idmgr, getKeyPair()).WillByDefault(ReturnRef(localKeyPair));
    ON_CALL(*marshaller, marshal(localKeyPair.publicKey))
        .WillByDefault(Return(std::vector<uint8_t>{1}));
  }

  std::shared_ptr<NiceMock<RawConnectionMock>> conn =
      std::make_shared<NiceMock<RawConnectionMock>>();

  PublicKey publicKey{{Key::Type::ED25519, {1}}};

  KeyPair localKeyPair{
      {{Key::Type::ED25519, {2}}},
      {{Key::Type::ED25519, {3}}},
  };
};

/**
 * @given plaintext security adaptor
 * @when getting id of the underlying security protocol
 * @then an expected id is returned
 */
TEST_F(PlaintextAdaptorTest, GetId) {
  ASSERT_EQ(adaptor->getProtocolId(), "/plaintext/1.0.0");
}

/**
 * @given plaintext security adaptor
 * @when securing a raw connection inbound, using that adaptor
 * @then connection is secured
 */
TEST_F(PlaintextAdaptorTest, SecureInbound) {
  adaptor->secureInbound(
      conn, [this](outcome::result<std::shared_ptr<SecureConnection>> rc) {
        EXPECT_OUTCOME_TRUE(sec, rc);

        EXPECT_OUTCOME_TRUE(remotePubkey, sec->remotePublicKey());
        EXPECT_EQ(remotePubkey, publicKey);

        EXPECT_OUTCOME_TRUE(remoteId, sec->remotePeer());
        auto calculated = PeerId::fromPublicKey(publicKey);

        EXPECT_EQ(remoteId, calculated);
      });
}

/**
 * @given plaintext security adaptor
 * @when securing a raw connection outbound, using that adaptor
 * @then connection is secured
 */
TEST_F(PlaintextAdaptorTest, SecureOutbound) {
  const PeerId pid = PeerId::fromPublicKey(publicKey);
  adaptor->secureOutbound(
      conn, pid,
      [pid, this](outcome::result<std::shared_ptr<SecureConnection>> rc) {
        EXPECT_OUTCOME_TRUE(sec, rc);

        EXPECT_OUTCOME_TRUE(remotePubkey, sec->remotePublicKey());
        EXPECT_EQ(remotePubkey, publicKey);

        EXPECT_OUTCOME_TRUE(remoteId, sec->remotePeer());
        auto calculated = PeerId::fromPublicKey(publicKey);

        EXPECT_EQ(remoteId, calculated);
        EXPECT_EQ(remoteId, pid);
      });
}
