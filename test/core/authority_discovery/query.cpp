/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authority_discovery/query/query_impl.hpp"

#include <gtest/gtest.h>
#include <mock/libp2p/basic/scheduler_mock.hpp>
#include <mock/libp2p/crypto/key_marshaller_mock.hpp>
#include <mock/libp2p/host/host_mock.hpp>
#include <mock/libp2p/peer/address_repository_mock.hpp>
#include <mock/libp2p/peer/peer_repository_mock.hpp>

#include "authority_discovery/publisher/address_publisher.hpp"
#include "authority_discovery/query/audi_store_impl.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/crypto/ed25519_provider_mock.hpp"
#include "mock/core/crypto/key_store_mock.hpp"
#include "mock/core/crypto/sr25519_provider_mock.hpp"
#include "mock/core/network/protocols/parachain.hpp"
#include "mock/core/runtime/authority_discovery_api_mock.hpp"
#include "mock/libp2p/crypto/crypto_provider.hpp"
#include "mock/libp2p/protocol/kademlia/kademlia_mock.hpp"
#include "testutil/lazy.hpp"
#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/storage/in_memory/in_memory_spaced_storage.hpp"

using kagome::application::AppStateManagerMock;
using kagome::authority_discovery::audiEncode;
using kagome::authority_discovery::AudiStoreImpl;
using kagome::authority_discovery::QueryImpl;
using kagome::blockchain::BlockTreeMock;
using kagome::crypto::Ed25519ProviderMock;
using kagome::crypto::KeyStoreMock;
using kagome::crypto::Sr25519Keypair;
using kagome::crypto::Sr25519ProviderMock;
using kagome::network::ValidationProtocolReserve;
using kagome::network::ValidationProtocolReserveMock;
using kagome::runtime::AuthorityDiscoveryApiMock;
using kagome::storage::InMemorySpacedStorage;
using libp2p::HostMock;
using libp2p::Multiaddress;
using libp2p::PeerId;
using libp2p::basic::SchedulerMock;
using libp2p::crypto::CryptoProviderMock;
using libp2p::crypto::ProtobufKey;
using libp2p::crypto::marshaller::KeyMarshallerMock;
using libp2p::peer::AddressRepositoryMock;
using libp2p::peer::PeerInfo;
using libp2p::peer::PeerRepositoryMock;
using libp2p::protocol::kademlia::Kademlia;
using libp2p::protocol::kademlia::KademliaMock;
using testing::_;
using testing::Return;
using testing::ReturnRef;
using testutil::sptr_to_lazy;

struct QueryTest : testing::Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    auto app_state_manager = std::make_shared<AppStateManagerMock>();
    EXPECT_CALL(*app_state_manager, atLaunch(_));
    EXPECT_CALL(*block_tree_, bestBlock());
    EXPECT_CALL(*api_, authorities(_))
        .WillRepeatedly(Return(std::vector{audi_key_.public_key}));
    EXPECT_CALL(*validation_protocol_, reserve(_, true))
        .Times(testing::AnyNumber())
        .WillRepeatedly([this](const PeerId &peer_id, bool) {
          reserved_.emplace(peer_id);
        });
    EXPECT_CALL(key_store_->sr25519(), getPublicKeys(_))
        .WillRepeatedly(Return(outcome::success()));
    EXPECT_CALL(*sr25519_provider_, sign(_, _))
        .WillRepeatedly(Return(outcome::success()));
    EXPECT_CALL(*sr25519_provider_, verify(_, _, _)).WillRepeatedly([this] {
      return sig_ok_;
    });
    EXPECT_CALL(*libp2p_crypto_provider_, verify(_, _, _))
        .WillRepeatedly([this] { return sig_ok_; });
    EXPECT_CALL(*ed25519_provider_, sign(_, _))
        .WillRepeatedly(Return(outcome::success()));
    EXPECT_CALL(*key_marshaller_, unmarshalPublicKey(_))
        .WillRepeatedly(Return(outcome::success()));
    EXPECT_CALL(*host_, getId()).WillRepeatedly(Return("b"_peerid));
    EXPECT_CALL(*host_, getPeerRepository())
        .WillRepeatedly(ReturnRef(peer_repo_));
    EXPECT_CALL(peer_repo_, getAddressRepository())
        .WillRepeatedly(ReturnRef(address_repo_));
    EXPECT_CALL(address_repo_, addAddresses(_, _, _))
        .WillRepeatedly(Return(outcome::success()));
    EXPECT_CALL(*scheduler_, scheduleImpl(_, _, _));
    query_ = std::make_shared<QueryImpl>(
        app_state_manager,
        block_tree_,
        api_,
        sptr_to_lazy<ValidationProtocolReserve>(validation_protocol_),
        key_store_,
        audi_store_,
        sr25519_provider_,
        libp2p_crypto_provider_,
        key_marshaller_,
        *host_,
        sptr_to_lazy<Kademlia>(kademlia_),
        scheduler_);
    query_->update().value();
  }

  auto info(size_t i) {
    auto addr = Multiaddress::create(
                    fmt::format("/tcp/{}/p2p/{}", i, peer_id_.toBase58()))
                    .value();
    return PeerInfo{peer_id_, {addr}};
  }

  void receive(size_t i, std::optional<int> time) {
    auto raw =
        audiEncode(ed25519_provider_,
                   sr25519_provider_,
                   {},
                   key_pb_,
                   info(i),
                   audi_key_,
                   time ? std::make_optional(std::chrono::nanoseconds{*time})
                        : std::nullopt)
            .value();
    std::ignore = query_->validate(raw.first, raw.second);
  }

  void expect(std::optional<size_t> i) {
    auto r = query_->get(audi_key_.public_key);
    EXPECT_EQ(r, i ? std::make_optional(info(*i)) : std::nullopt);
  }

  std::shared_ptr<BlockTreeMock> block_tree_ =
      std::make_shared<BlockTreeMock>();
  std::shared_ptr<AuthorityDiscoveryApiMock> api_ =
      std::make_shared<AuthorityDiscoveryApiMock>();
  std::shared_ptr<ValidationProtocolReserveMock> validation_protocol_ =
      std::make_shared<ValidationProtocolReserveMock>();
  std::shared_ptr<KeyStoreMock> key_store_ = std::make_shared<KeyStoreMock>();
  std::shared_ptr<AudiStoreImpl> audi_store_ = std::make_shared<AudiStoreImpl>(
      std::make_shared<InMemorySpacedStorage>());
  std::shared_ptr<Sr25519ProviderMock> sr25519_provider_ =
      std::make_shared<Sr25519ProviderMock>();
  std::shared_ptr<CryptoProviderMock> libp2p_crypto_provider_ =
      std::make_shared<CryptoProviderMock>();
  std::shared_ptr<KeyMarshallerMock> key_marshaller_ =
      std::make_shared<KeyMarshallerMock>();
  std::shared_ptr<HostMock> host_ = std::make_shared<HostMock>();
  std::shared_ptr<KademliaMock> kademlia_ = std::make_shared<KademliaMock>();
  std::shared_ptr<SchedulerMock> scheduler_ = std::make_shared<SchedulerMock>();
  std::shared_ptr<QueryImpl> query_;
  Sr25519Keypair audi_key_;
  ProtobufKey key_pb_{{0, 1}};
  PeerId peer_id_ = PeerId::fromPublicKey(key_pb_).value();
  std::shared_ptr<Ed25519ProviderMock> ed25519_provider_ =
      std::make_shared<Ed25519ProviderMock>();
  AddressRepositoryMock address_repo_;
  PeerRepositoryMock peer_repo_;
  bool sig_ok_ = true;
  std::set<PeerId> reserved_;
};

// https://github.com/paritytech/polkadot-sdk/blob/da2dd9b7737cb7c0dc9dc3dc74b384c719ea3306/polkadot/node/network/gossip-support/src/tests.rs#L812
/**
 * @given record about peer
 * @when receive record
 * @then connect to peer
 */
TEST_F(QueryTest, test_quickly_connect_to_authorities_that_changed_address) {
  receive(1, std::nullopt);
  EXPECT_TRUE(reserved_.contains(peer_id_));
}

// https://github.com/paritytech/polkadot-sdk/blob/da2dd9b7737cb7c0dc9dc3dc74b384c719ea3306/substrate/client/authority-discovery/src/worker/tests.rs#L707
/**
 * @given record without timestamp
 * @when receive record
 * @then record is inserted
 */
TEST_F(QueryTest, strict_accept_address_without_creation_time) {
  expect(std::nullopt);
  receive(1, std::nullopt);
  expect(1);
}

// https://github.com/paritytech/polkadot-sdk/blob/da2dd9b7737cb7c0dc9dc3dc74b384c719ea3306/substrate/client/authority-discovery/src/worker/tests.rs#L728
/**
 * @given old record
 * @when receive new record
 * @then new record overwrites old record
 */
TEST_F(QueryTest, keep_last_received_if_no_creation_time) {
  receive(1, std::nullopt);
  receive(2, std::nullopt);
  expect(2);
}

// https://github.com/paritytech/polkadot-sdk/blob/da2dd9b7737cb7c0dc9dc3dc74b384c719ea3306/substrate/client/authority-discovery/src/worker/tests.rs#L775
/**
 * @given record with invalid signature
 * @when receive record
 * @then record ignored
 */
TEST_F(QueryTest, records_with_incorrectly_signed_creation_time_are_ignored) {
  sig_ok_ = false;
  receive(1, std::nullopt);
  expect(std::nullopt);
}

// https://github.com/paritytech/polkadot-sdk/blob/da2dd9b7737cb7c0dc9dc3dc74b384c719ea3306/substrate/client/authority-discovery/src/worker/tests.rs#L829
/**
 * @given old record
 * @when receive new record
 * @then new record overwrites old record
 */
TEST_F(QueryTest, newer_records_overwrite_older_ones) {
  receive(1, 1);
  receive(2, 2);
  expect(2);
}

// https://github.com/paritytech/polkadot-sdk/blob/da2dd9b7737cb7c0dc9dc3dc74b384c719ea3306/substrate/client/authority-discovery/src/worker/tests.rs#L880
/**
 * @given new record
 * @when receive old record
 * @then old record is ignored
 */
TEST_F(QueryTest, older_records_dont_affect_newer_ones) {
  receive(2, 2);
  receive(1, 1);
  expect(2);
}
