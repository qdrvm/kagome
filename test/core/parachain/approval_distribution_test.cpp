/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "core/parachain/parachain_test_harness.hpp"
#include "injector/bind_by_lambda.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/authority_discovery/query_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "common/main_thread_pool.hpp"
#include "common/worker_thread_pool.hpp"
#include "mock/core/consensus/babe/babe_config_repository_mock.hpp"
#include "mock/core/consensus/timeline/slots_util_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/crypto/key_store_mock.hpp"
#include "mock/core/crypto/sr25519_provider_mock.hpp"
#include "mock/core/dispute_coordinator/dispute_coordinator_mock.hpp"
#include "mock/core/network/peer_manager_mock.hpp"
#include "mock/core/network/peer_view_mock.hpp"
#include "mock/core/network/router_mock.hpp"
#include "mock/core/parachain/parachain_processor_mock.hpp"
#include "mock/core/parachain/pvf_mock.hpp"
#include "mock/core/parachain/recovery_mock.hpp"
#include "primitives/event_types.hpp"
#include "mock/core/runtime/parachain_host_mock.hpp"
#include "network/types/collator_messages_vstaging.hpp"
#include "parachain/approval/approval_distribution.hpp"
#include "parachain/backing/grid.hpp"
#include "testutil/lazy.hpp"
#include "testutil/prepare_loggers.hpp"
#include "libp2p/crypto/random_generator.hpp"
#include "libp2p/peer/peer_id.hpp"

using kagome::application::AppStateManagerMock;
using kagome::authority_discovery::QueryMock;
using kagome::blockchain::BlockTreeMock;
using kagome::common::MainThreadPool;
using kagome::common::WorkerThreadPool;
using kagome::consensus::SlotsUtilMock;
using kagome::consensus::babe::BabeConfigRepositoryMock;
using kagome::crypto::HasherMock;
using kagome::crypto::KeyStoreMock;
using kagome::crypto::Sr25519ProviderMock;
using kagome::dispute::DisputeCoordinatorMock;
using kagome::network::PeerManagerMock;
using kagome::network::PeerViewMock;
using kagome::network::Router;
using kagome::network::RouterMock;
using kagome::network::vstaging::Assignment;
using kagome::network::vstaging::Assignments;
using kagome::network::vstaging::Approvals;
using kagome::parachain::ApprovalDistribution;
using kagome::parachain::approval::IndirectAssignmentCertV2;
using kagome::parachain::ParachainProcessorMock;
using kagome::parachain::PvfMock;
using kagome::parachain::RecoveryMock;
using kagome::primitives::events::ChainSubscriptionEngine;
using kagome::runtime::ParachainHostMock;
using kagome::runtime::SessionInfo;
using kagome::Watchdog;
using testing::_;
using testing::Return;
using testing::SaveArg;

// Define a fake ValidationProtocol interface that matches what we need
namespace kagome::network {
  class ValidationProtocol {
   public:
    virtual ~ValidationProtocol() = default;
    virtual bool write(const std::unordered_set<libp2p::peer::PeerId> &, 
                      const vstaging::Assignments &) = 0;
    virtual bool write(const std::unordered_set<libp2p::peer::PeerId> &, 
                      const vstaging::Approvals &) = 0;
  };
}

// Create a mock for ValidationProtocol
class ValidationProtocolMock : public kagome::network::ValidationProtocol {
 public:
  MOCK_METHOD(bool, 
              write, 
              (const std::unordered_set<libp2p::peer::PeerId> &, 
               const Assignments &), 
              (override));
  
  MOCK_METHOD(bool, 
              write, 
              (const std::unordered_set<libp2p::peer::PeerId> &, 
               const Approvals &), 
              (override));
};

class ApprovalDistributionTest : public ProspectiveParachainsTestHarness {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    ProspectiveParachainsTestHarness::SetUp();
    
    // Setup watchdog and thread pools
    watchdog_ = std::make_shared<Watchdog>(std::chrono::milliseconds(1));
    main_thread_pool_ = std::make_shared<MainThreadPool>(
        watchdog_, std::make_shared<boost::asio::io_context>());
    worker_thread_pool_ = std::make_shared<WorkerThreadPool>(watchdog_, 1);
    
    // Setup additional mocks
    app_state_manager_ = std::make_shared<AppStateManagerMock>();
    chain_sub_engine_ = std::make_shared<ChainSubscriptionEngine>();
    parachain_host_ = std::make_shared<ParachainHostMock>();
    keystore_ = std::make_shared<KeyStoreMock>();
    hasher_ = std::make_shared<HasherMock>();
    peer_view_ = std::make_shared<PeerViewMock>();
    parachain_processor_ = std::make_shared<ParachainProcessorMock>();
    sr25519_provider_ = std::make_shared<Sr25519ProviderMock>();
    peer_manager_ = std::make_shared<PeerManagerMock>();
    router_ = std::make_shared<RouterMock>();
    validation_protocol_ = std::make_shared<ValidationProtocolMock>();
    pvf_ = std::make_shared<PvfMock>();
    recovery_ = std::make_shared<RecoveryMock>();
    dispute_coordinator_ = std::make_shared<DisputeCoordinatorMock>();
    query_ = std::make_shared<QueryMock>();
    babe_config_repo_ = std::make_shared<BabeConfigRepositoryMock>();
    slots_util_ = std::make_shared<SlotsUtilMock>();

    // Mock keypair generation
    kagome::crypto::Sr25519Keypair keypair;
    std::array<uint8_t, 32> public_key_data{1, 2, 3};
    std::array<uint8_t, 64> secret_key_data{4, 5, 6};
    
    // Create Blob<32> for public key and Blob<64> for secret key
    kagome::common::Blob<32> public_key_blob;
    std::copy(public_key_data.begin(), public_key_data.end(), public_key_blob.begin());
    
    kagome::common::Blob<64> secret_key_blob;
    std::copy(secret_key_data.begin(), secret_key_data.end(), secret_key_blob.begin());
    
    // Properly initialize the public and secret keys
    keypair.public_key = kagome::crypto::Sr25519PublicKey(public_key_blob);
    keypair.secret_key = kagome::crypto::Sr25519SecretKey(secret_key_blob);
    
    EXPECT_CALL(*sr25519_provider_, generateKeypair(_, _))
        .WillRepeatedly(Return(keypair));

    // Configure mocks
    ON_CALL(*router_, getValidationProtocol())
        .WillByDefault(Return(std::static_pointer_cast<kagome::network::ValidationProtocol>(validation_protocol_)));
    ON_CALL(*parachain_processor_, canProcessParachains())
        .WillByDefault(Return(outcome::success()));
        
    // Setup grid topology with validator indices and peer IDs
    setupGridTopology();

    // Create and initialize ApprovalDistribution
    approval_distribution_ = std::make_shared<ApprovalDistribution>(
        babe_config_repo_,
        app_state_manager_,
        *chain_sub_engine_,
        *worker_thread_pool_,
        parachain_host_,
        testutil::sptr_to_lazy<kagome::consensus::SlotsUtil>(slots_util_),
        keystore_,
        hasher_,
        peer_view_,
        parachain_processor_,
        sr25519_provider_,
        peer_manager_,
        router_,
        block_tree_,
        pvf_,
        recovery_,
        approval_thread_pool_,
        *main_thread_pool_,
        kagome::LazySPtr<kagome::dispute::DisputeCoordinator>(dispute_coordinator_),
        query_);
        
    approval_distribution_->tryStart();
  }

  void TearDown() override {
    // Clean up resources
    approval_distribution_.reset();
    watchdog_->stop();
    ProspectiveParachainsTestHarness::TearDown();
  }
  
  void setupGridTopology() {
    // Create relay block hash
    relay_block_hash_ = hashFromStrData("0xdeadbeef");
    
    // Create keypair for tests
    std::array<uint8_t, 32> public_key_data{1, 2, 3};
    
    // Create Blob<32> for public key
    kagome::common::Blob<32> public_key_blob;
    std::copy(public_key_data.begin(), public_key_data.end(), public_key_blob.begin());
    
    // Properly initialize the public key
    kagome::crypto::Sr25519PublicKey public_key(public_key_blob);
    
    // Create validators and peers for X dimension (rows)
    for (size_t i = 0; i < 8; i++) {
      size_t validator_index = i * 10; // Validator indices: 0, 10, 20, 30, etc.
      
      // Generate peer ID
      auto peer_id = generateTestPeerId(i);
      
      x_validator_indices_.push_back(validator_index);
      x_peers_.push_back(peer_id);
      
      // Create Blob<32> for validator
      kagome::common::Blob<32> validator_key_blob;
      std::copy(public_key_data.begin(), public_key_data.end(), validator_key_blob.begin());
      
      validator_keys_[validator_index] = kagome::primitives::AuthorityDiscoveryId(validator_key_blob);
      peer_to_validator_[peer_id] = validator_index;
      
      // Setup authority discovery to map validators to peers
      libp2p::peer::PeerInfo peer_info{peer_id, {}};
      ON_CALL(*query_, get(validator_keys_[validator_index]))
          .WillByDefault(Return(peer_info));
      ON_CALL(*query_, get(peer_id))
          .WillByDefault(Return(validator_keys_[validator_index]));
          
      // Setup peer connections
      ON_CALL(*peer_manager_, isPeerConnected(peer_id))
          .WillByDefault(Return(true));
    }
    
    // Create validators and peers for Y dimension (columns)
    for (size_t i = 0; i < 8; i++) {
      size_t validator_index = 50 + i; // Validator indices: 50, 51, 52, etc.
      
      // Generate peer ID
      auto peer_id = generateTestPeerId(100 + i);
      
      y_validator_indices_.push_back(validator_index);
      y_peers_.push_back(peer_id);
      
      // Create Blob<32> for validator
      kagome::common::Blob<32> validator_key_blob;
      std::copy(public_key_data.begin(), public_key_data.end(), validator_key_blob.begin());
      
      validator_keys_[validator_index] = kagome::primitives::AuthorityDiscoveryId(validator_key_blob);
      peer_to_validator_[peer_id] = validator_index;
      
      // Setup authority discovery to map validators to peers
      libp2p::peer::PeerInfo peer_info{peer_id, {}};
      ON_CALL(*query_, get(validator_keys_[validator_index]))
          .WillByDefault(Return(peer_info));
      ON_CALL(*query_, get(peer_id))
          .WillByDefault(Return(validator_keys_[validator_index]));
          
      // Setup peer connections
      ON_CALL(*peer_manager_, isPeerConnected(peer_id))
          .WillByDefault(Return(true));
    }
    
    // Set up session info
    session_info_.n_cores = 4;
    session_info_.n_delay_tranches = 40;
    session_info_.zeroth_delay_tranche_width = 2;
    session_info_.relay_vrf_modulo_samples = 3;
    session_info_.needed_approvals = 2;
    session_info_.no_show_slots = 2;
    
    // Set up validator groups (not important for these tests)
    session_info_.validator_groups.resize(2);
    
    // Add all validators to the session info
    for (const auto &[index, key] : validator_keys_) {
      // Ensure session_info_ vectors have enough space
      while (session_info_.validators.size() <= index) {
        session_info_.validators.push_back({});
        session_info_.assignment_keys.push_back({});
      }
      session_info_.validators[index] = key;
      session_info_.assignment_keys[index] = key;
    }
    
    // Set up parachain host to return session info
    ON_CALL(*parachain_host_, session_info(_, _))
        .WillByDefault(Return(session_info_));
    
    // Set up peer views to include our test block
    for (const auto &peer : x_peers_) {
      peer_views_[peer] = {.heads_ = {relay_block_hash_}, .finalized_number_ = 0};
    }
    for (const auto &peer : y_peers_) {
      peer_views_[peer] = {.heads_ = {relay_block_hash_}, .finalized_number_ = 0};
    }
    
    // Setup peer view
    ON_CALL(*peer_view_, peersCount())
        .WillByDefault(Return(x_peers_.size() + y_peers_.size()));
        
    // Mock peer view to return views for all peers
    for (const auto &[peer, view] : peer_views_) {
      ON_CALL(*peer_view_, getPeerView(peer))
          .WillByDefault(Return(view));
    }
  }

  // Helper to create deterministic test peer IDs
  libp2p::peer::PeerId generateTestPeerId(size_t index) {
    // Convert index to string
    std::string id_str = "test-peer-id-" + std::to_string(index);
    
    // Create a SHA-256 hash
    std::array<uint8_t, 32> hash_data{};
    std::copy_n(id_str.begin(), std::min(id_str.size(), hash_data.size()), hash_data.begin());
    
    // Create a multihash and peer ID
    auto multihash = libp2p::multi::Multihash::create(libp2p::multi::HashType::sha256, hash_data).value();
    return libp2p::peer::PeerId::fromMultihash(multihash).value();
  }

  // Create a candidate bitfield with a single bit set
  scale::BitVector createCandidateBitfield(uint32_t candidate_index) {
    scale::BitVector bitfield;
    bitfield.resize(candidate_index + 1);
    bitfield[candidate_index] = true;
    return bitfield;
  }
  
  // Test if a peer is in the specified peers set
  bool isPeerInSet(const libp2p::peer::PeerId& peer, 
                  const std::unordered_set<libp2p::peer::PeerId>& peers) {
    return peers.find(peer) != peers.end();
  }
  
  // Creates a dummy assignment for testing
  IndirectAssignmentCertV2 createAssignment(
      size_t validator_index, 
      const Hash& block_hash = Hash()) {
    IndirectAssignmentCertV2 assignment;
    assignment.block_hash = block_hash.empty() ? relay_block_hash_ : block_hash;
    assignment.validator = validator_index;
    assignment.cert.kind = kagome::parachain::approval::RelayVRFDelay{.core_index = 0};
    return assignment;
  }

 protected:
  // Test data
  Hash relay_block_hash_;
  std::vector<size_t> x_validator_indices_;
  std::vector<size_t> y_validator_indices_;
  std::vector<libp2p::peer::PeerId> x_peers_;
  std::vector<libp2p::peer::PeerId> y_peers_;
  std::map<size_t, kagome::primitives::AuthorityDiscoveryId> validator_keys_;
  std::map<libp2p::peer::PeerId, size_t> peer_to_validator_;
  std::map<libp2p::peer::PeerId, kagome::network::View> peer_views_;
  SessionInfo session_info_;
  
  // Thread handling
  std::shared_ptr<Watchdog> watchdog_;
  
  // Mock objects
  std::shared_ptr<AppStateManagerMock> app_state_manager_;
  std::shared_ptr<ChainSubscriptionEngine> chain_sub_engine_;
  std::shared_ptr<WorkerThreadPool> worker_thread_pool_;
  std::shared_ptr<ParachainHostMock> parachain_host_;
  std::shared_ptr<KeyStoreMock> keystore_;
  std::shared_ptr<HasherMock> hasher_;
  std::shared_ptr<PeerViewMock> peer_view_;
  std::shared_ptr<ParachainProcessorMock> parachain_processor_;
  std::shared_ptr<Sr25519ProviderMock> sr25519_provider_;
  std::shared_ptr<PeerManagerMock> peer_manager_;
  std::shared_ptr<RouterMock> router_;
  std::shared_ptr<ValidationProtocolMock> validation_protocol_;
  std::shared_ptr<PvfMock> pvf_;
  std::shared_ptr<RecoveryMock> recovery_;
  std::shared_ptr<MainThreadPool> main_thread_pool_;
  std::shared_ptr<DisputeCoordinatorMock> dispute_coordinator_;
  std::shared_ptr<QueryMock> query_;
  std::shared_ptr<BabeConfigRepositoryMock> babe_config_repo_;
  std::shared_ptr<SlotsUtilMock> slots_util_;
  kagome::parachain::ApprovalThreadPool approval_thread_pool_;
  
  // The object being tested
  std::shared_ptr<ApprovalDistribution> approval_distribution_;
};

/**
 * Test that locally generated assignments are propagated to both dimensions
 * (X and Y dimensions) in the grid topology.
 */
TEST_F(ApprovalDistributionTest, propagates_locally_generated_assignment_to_both_dimensions) {
  // Capture which peers receive the assignment
  std::unordered_set<libp2p::peer::PeerId> assignment_peers;
  EXPECT_CALL(*validation_protocol_, write(testing::_, testing::An<const Assignments&>()))
      .WillOnce(testing::DoAll(
          SaveArg<0>(&assignment_peers),
          Return(true)));
  
  // Create an assignment and candidate bitfield
  auto assignment = createAssignment(0); // Validator index doesn't matter for local assignments
  auto candidate_bitfield = createCandidateBitfield(0);
  
  // Call import_and_circulate_assignment with source=nullopt to simulate a local assignment
  approval_distribution_->import_and_circulate_assignment(std::nullopt, assignment, candidate_bitfield);
  
  // Check that all X dimension peers received the assignment
  for (const auto &peer : x_peers_) {
    EXPECT_TRUE(isPeerInSet(peer, assignment_peers))
        << "X dimension peer " << peer.toBase58() << " did not receive the assignment";
  }
  
  // Check that all Y dimension peers received the assignment
  for (const auto &peer : y_peers_) {
    EXPECT_TRUE(isPeerInSet(peer, assignment_peers))
        << "Y dimension peer " << peer.toBase58() << " did not receive the assignment";
  }
}

/**
 * Test that assignments from a validator in the X dimension are
 * propagated to peers in the Y dimension.
 */
TEST_F(ApprovalDistributionTest, propagates_assignments_along_unshared_dimension_from_x_to_y) {
  // Use a validator from X dimension
  size_t validator_index = x_validator_indices_[0]; // First validator in X dimension
  
  // Create an assignment from the X dimension validator
  auto assignment = createAssignment(validator_index);
  auto candidate_bitfield = createCandidateBitfield(0);
  
  // Get peer ID of the source
  libp2p::peer::PeerId source_peer = x_peers_[0]; // Corresponding to the first validator
  
  // Capture which peers receive the assignment
  std::unordered_set<libp2p::peer::PeerId> assignment_peers;
  EXPECT_CALL(*validation_protocol_, write(testing::_, testing::An<const Assignments&>()))
      .WillOnce(testing::DoAll(
          SaveArg<0>(&assignment_peers),
          Return(true)));
  
  // Call import_and_circulate_assignment with source from X dimension
  approval_distribution_->import_and_circulate_assignment(source_peer, assignment, candidate_bitfield);
  
  // Verify that Y dimension peers received the assignment
  for (const auto &peer : y_peers_) {
    EXPECT_TRUE(isPeerInSet(peer, assignment_peers))
        << "Y dimension peer " << peer.toBase58() << " should have received the assignment";
  }
  
  // The source peer should not receive the assignment back
  EXPECT_FALSE(isPeerInSet(source_peer, assignment_peers))
      << "Source peer " << source_peer.toBase58() << " should not have received the assignment";
}

/**
 * Test that assignments from a validator in the Y dimension are
 * propagated to peers in the X dimension.
 */
TEST_F(ApprovalDistributionTest, propagates_assignments_along_unshared_dimension_from_y_to_x) {
  // Use a validator from Y dimension
  size_t validator_index = y_validator_indices_[0]; // First validator in Y dimension
  
  // Create an assignment from the Y dimension validator
  auto assignment = createAssignment(validator_index);
  auto candidate_bitfield = createCandidateBitfield(0);
  
  // Get peer ID of the source
  libp2p::peer::PeerId source_peer = y_peers_[0]; // Corresponding to the first validator
  
  // Capture which peers receive the assignment
  std::unordered_set<libp2p::peer::PeerId> assignment_peers;
  EXPECT_CALL(*validation_protocol_, write(testing::_, testing::An<const Assignments&>()))
      .WillOnce(testing::DoAll(
          SaveArg<0>(&assignment_peers),
          Return(true)));
  
  // Call import_and_circulate_assignment with source from Y dimension
  approval_distribution_->import_and_circulate_assignment(source_peer, assignment, candidate_bitfield);
  
  // Verify that X dimension peers received the assignment
  for (const auto &peer : x_peers_) {
    EXPECT_TRUE(isPeerInSet(peer, assignment_peers))
        << "X dimension peer " << peer.toBase58() << " should have received the assignment";
  }
  
  // The source peer should not receive the assignment back
  EXPECT_FALSE(isPeerInSet(source_peer, assignment_peers))
      << "Source peer " << source_peer.toBase58() << " should not have received the assignment";
} 