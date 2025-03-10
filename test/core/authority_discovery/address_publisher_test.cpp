/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authority_discovery/publisher/address_publisher.hpp"

#include <gtest/gtest.h>
#include <mock/libp2p/basic/scheduler_mock.hpp>
#include <mock/libp2p/crypto/key_marshaller_mock.hpp>
#include <mock/libp2p/host/host_mock.hpp>

#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/crypto/ed25519_provider_mock.hpp"
#include "mock/core/crypto/key_store_mock.hpp"
#include "mock/core/crypto/sr25519_provider_mock.hpp"
#include "mock/core/runtime/authority_discovery_api_mock.hpp"
#include "mock/libp2p/protocol/kademlia/kademlia_mock.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::application::AppConfigurationMock;
using kagome::application::AppStateManagerMock;
using kagome::authority_discovery::AddressPublisher;
using kagome::blockchain::BlockTreeMock;
using kagome::crypto::Ed25519PrivateKey;
using kagome::crypto::Ed25519ProviderMock;
using kagome::crypto::Ed25519PublicKey;
using kagome::crypto::KeyStoreMock;
using kagome::crypto::SecureBuffer;
using kagome::crypto::SessionKeysImpl;
using kagome::crypto::Sr25519Keypair;
using kagome::crypto::Sr25519ProviderMock;
using kagome::crypto::Sr25519PublicKey;
using kagome::crypto::Sr25519SecretKey;
using kagome::network::Roles;
using kagome::runtime::AuthorityDiscoveryApiMock;
using libp2p::HostMock;
using libp2p::basic::SchedulerMock;
using libp2p::crypto::marshaller::KeyMarshallerMock;
using libp2p::multi::Multiaddress;
using libp2p::peer::PeerId;
using libp2p::peer::PeerInfo;
using libp2p::protocol::kademlia::KademliaMock;
using testing::_;
using testing::Return;

struct AddressPublisherTest : public testing::Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    roles_ = Roles::Authority;
    EXPECT_CALL(*config_, roles()).WillOnce(Return(roles_));
    session_keys_ = std::make_shared<SessionKeysImpl>(crypto_store_, *config_);
    libp2p_key_.privateKey.type = libp2p::crypto::Key::Type::Ed25519;
    libp2p_key_.privateKey.data.resize(Ed25519PrivateKey::size());
    libp2p_key_.publicKey.data.resize(Ed25519PublicKey::size());
    peer_info_.addresses.push_back(
        Multiaddress::create("/ip4/127.0.0.1").value());

    EXPECT_CALL(*app_state_manager_, atLaunch(_));
    EXPECT_CALL(*key_marshaller_, marshal(libp2p_key_.publicKey))
        .WillOnce(Return(libp2p::crypto::ProtobufKey{{}}));

    publisher_ = std::make_shared<AddressPublisher>(authority_discovery_api_,
                                                    roles_,
                                                    app_state_manager_,
                                                    block_tree_,
                                                    session_keys_,
                                                    libp2p_key_,
                                                    *key_marshaller_,
                                                    ed25519_provider_,
                                                    sr25519_provider_,
                                                    *host_,
                                                    kademlia_,
                                                    scheduler_);
  }

  std::shared_ptr<AppConfigurationMock> config_ =
      std::make_shared<AppConfigurationMock>();
  std::shared_ptr<AuthorityDiscoveryApiMock> authority_discovery_api_ =
      std::make_shared<AuthorityDiscoveryApiMock>();
  Roles roles_;
  std::shared_ptr<AppStateManagerMock> app_state_manager_ =
      std::make_shared<AppStateManagerMock>();
  std::shared_ptr<BlockTreeMock> block_tree_ =
      std::make_shared<BlockTreeMock>();
  std::shared_ptr<SessionKeysImpl> session_keys_;
  libp2p::crypto::KeyPair libp2p_key_;
  std::shared_ptr<KeyMarshallerMock> key_marshaller_ =
      std::make_shared<KeyMarshallerMock>();
  std::shared_ptr<Ed25519ProviderMock> ed25519_provider_ =
      std::make_shared<Ed25519ProviderMock>();
  std::shared_ptr<Sr25519ProviderMock> sr25519_provider_ =
      std::make_shared<Sr25519ProviderMock>();
  std::shared_ptr<HostMock> host_ = std::make_shared<HostMock>();
  std::shared_ptr<KademliaMock> kademlia_ = std::make_shared<KademliaMock>();
  std::shared_ptr<SchedulerMock> scheduler_ = std::make_shared<SchedulerMock>();
  std::shared_ptr<KeyStoreMock> crypto_store_ =
      std::make_shared<KeyStoreMock>();
  PeerInfo peer_info_{
      PeerId::fromBase58("12D3KooWGYLoNGrZn2nwewBiPFZuKHZebPDL9QAF26cVgLxwuiTZ")
          .value(),
      {}};

  Sr25519PublicKey audi_key_;
  std::shared_ptr<AddressPublisher> publisher_;
};

/**
 * @given address publisher
 * @when publish
 * @then success
 */
TEST_F(AddressPublisherTest, Success) {
  EXPECT_CALL(*host_, getPeerInfo()).WillOnce(Return(peer_info_));
  EXPECT_CALL(crypto_store_->sr25519(), getPublicKeys(_))
      .WillOnce(Return(std::vector{audi_key_}));
  EXPECT_CALL(crypto_store_->sr25519(), findKeypair(_, _))
      .WillOnce(Return(Sr25519Keypair{}));
  EXPECT_CALL(*block_tree_, bestBlock());
  EXPECT_CALL(*authority_discovery_api_, authorities(_))
      .WillOnce(Return(std::vector{audi_key_}));
  EXPECT_CALL(*ed25519_provider_, sign(_, _))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*sr25519_provider_, sign(_, _))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*kademlia_, putValue(_, _)).WillOnce(Return(outcome::success()));

  publisher_->publishOwnAddress().value();
}
