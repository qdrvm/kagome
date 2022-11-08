/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPROVAL_DISTRIBUTION_HPP
#define KAGOME_APPROVAL_DISTRIBUTION_HPP

#include <unordered_set>
#include <vector>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/variant.hpp>
#include <clock/timer.hpp>

#include "blockchain/block_tree.hpp"
#include "consensus/babe/babe_util.hpp"
#include "consensus/babe/common.hpp"
#include "consensus/babe/types/babe_block_header.hpp"
#include "crypto/crypto_store/key_file_storage.hpp"
#include "crypto/crypto_store/session_keys.hpp"
#include "network/peer_view.hpp"
#include "network/types/collator_messages.hpp"
#include "parachain/approval/store.hpp"
#include "parachain/thread_pool.hpp"
#include "runtime/runtime_api/parachain_host.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"
#include "utils/safe_object.hpp"

namespace kagome::parachain {
  using DistributeAssignment = network::Assignment;
  using ApprovalDistributionSubsystemMsg = boost::variant<
      std::vector<approval::BlockApprovalMeta>,  /// Notify the
                                                 /// `ApprovalDistribution`
                                                 /// subsystem about new blocks
                                                 /// and the candidates
                                                 /// contained within them.
      DistributeAssignment,  /// Distribute an assignment cert from the local
                             /// validator. The cert is assumed to be valid,
                             /// relevant, and for the given relay-parent and
                             /// validator index.
      network::IndirectSignedApprovalVote,  /// Distribute an approval vote for
                                            /// the local validator. The
                                            /// approval vote is assumed to be
                                            /// valid, relevant, and the
                                            /// corresponding approval already
                                            /// issued. If not, the subsystem is
                                            /// free to drop the message.
      network::ApprovalDistributionMessage  /// An update from the network
                                            /// bridge.
      >;

  /// The approval voting subsystem.
  struct ApprovalVotingSubsystem {
    uint64_t slot_duration_millis;
  };

  /**
   * The approval voting process ensures that only valid parachain blocks are
   * finalized on the relay chain. After backable parachain candidates were
   * submitted to the relay chain, which can be retrieved via the Runtime API,
   * validators need to determine their assignments for each parachain and issue
   * approvals for valid candidates, respectively disputes for invalid
   * candidates.
   */
  struct ApprovalDistribution final
      : public std::enable_shared_from_this<ApprovalDistribution> {
    enum class Error {
      NO_INSTANCE = 1,
      NO_CONTEXT = 2,
      NO_SESSION_INFO = 3,
      UNUSED_SLOT_TYPE = 4
    };

    struct OurAssignment {
      approval::AssignmentCert cert;
      uint32_t tranche;
      ValidatorIndex validator_index;
      bool triggered;  /// Whether the assignment has been triggered already.
    };

    ApprovalDistribution(
        std::shared_ptr<thread::ThreadPool> thread_pool,
        std::shared_ptr<runtime::ParachainHost> parachain_host,
        std::shared_ptr<consensus::BabeUtil> babe_util,
        std::shared_ptr<crypto::CryptoStore> keystore,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<const blockchain::BlockTree> block_tree,
        std::shared_ptr<boost::asio::io_context> this_context,
        const ApprovalVotingSubsystem &config,
        std::shared_ptr<network::PeerView> peer_view,
        std::unique_ptr<clock::Timer> timer);
    ~ApprovalDistribution();

    /// AppStateManager impl
    bool prepare();
    bool start();
    void stop();

   private:
    using CandidateIncludedList =
        std::vector<std::tuple<CandidateHash,
                               network::CandidateReceipt,
                               CoreIndex,
                               GroupIndex>>;
    using AssignmentsList = std::unordered_map<CoreIndex, OurAssignment>;

    struct ImportedBlockInfo {
      CandidateIncludedList included_candidates;
      SessionIndex session_index;
      AssignmentsList assignments;
      size_t n_validators;
      RelayVRFStory relay_vrf_story;
      consensus::BabeSlotNumber slot;
      runtime::SessionInfo session_info;
      std::optional<primitives::BlockNumber> force_approve;
    };

    struct ApprovingContext {
      primitives::BlockHeader block_header;
      std::optional<CandidateIncludedList> included_candidates;
      std::optional<SessionIndex> session_index;
      std::optional<consensus::BabeBlockHeader> babe_block_header;
      std::optional<consensus::EpochNumber> babe_epoch;
      std::optional<runtime::SessionInfo> session_info;

      std::shared_ptr<boost::asio::io_context> complete_callback_context;
      std::function<void(outcome::result<ImportedBlockInfo> &&)>
          complete_callback;

      bool is_complete() const {
        return included_candidates && babe_epoch && session_index
               && session_info && babe_block_header;
      }
    };

    /// Metadata regarding a specific tranche of assignments for a specific
    /// candidate.
    struct TrancheEntry {
      network::DelayTranche tranche;
      // Assigned validators, and the instant we received their assignment,
      // rounded to the nearest tick.
      std::vector<std::pair<ValidatorIndex, Tick>> assignments;
    };

    struct ApprovalEntry {
      std::vector<TrancheEntry> tranches;
      GroupIndex backing_group;
      std::optional<OurAssignment> our_assignment;
      std::optional<ValidatorSignature> our_approval_sig;
      // `n_validators` bits.
      scale::BitVec assignments;
      bool approved;

      ApprovalEntry(
          GroupIndex group_index,
          std::optional<std::reference_wrapper<OurAssignment>> assignment,
          size_t assignments_size)
          : backing_group{group_index},
            our_assignment{assignment},
            approved(false) {
        assignments.bits.insert(
            assignments.bits.begin(), assignments_size, false);
      }
    };

    struct CandidateEntry {
      network::CandidateReceipt candidate;
      SessionIndex session;
      // Assignments are based on blocks, so we need to track assignments
      // separately based on the block we are looking at.
      std::unordered_map<network::Hash, ApprovalEntry> block_assignments;
      scale::BitVec approvals;

      CandidateEntry(const network::CandidateReceipt &receipt,
                     SessionIndex session_index,
                     size_t approvals_size)
          : candidate(receipt), session(session_index) {
        approvals.bits.insert(approvals.bits.begin(), approvals_size, false);
      }
    };

    /// Information about a block and imported candidates.
    struct BlockImportedCandidates {
      primitives::BlockHash block_hash;
      primitives::BlockNumber block_number;
      network::Tick block_tick;
      network::Tick no_show_duration;
      std::vector<std::pair<CandidateHash, CandidateEntry>> imported_candidates;
    };

    using ApprovingContextMap =
        std::unordered_map<primitives::BlockHash, ApprovingContext>;
    using ApprovingContextUnit = ApprovingContextMap::iterator::value_type;

    AssignmentsList compute_assignments(
        std::shared_ptr<crypto::CryptoStore> const &keystore,
        runtime::SessionInfo const &config,
        RelayVRFStory const &relay_vrf_story,
        CandidateIncludedList const &leaving_cores);

    void imported_block_info(
        const primitives::BlockHash &block_hash,
        const primitives::BlockHeader &block_header,
        const std::shared_ptr<boost::asio::io_context> &callback_exec_context,
        std::function<void(outcome::result<ImportedBlockInfo> &&)> &&callback);

    template <typename Func>
    void handle_new_head(const primitives::BlockHash &head,
                         primitives::BlockNumber finalized,
                         Func &&func);

    void request_included_candidates(const primitives::BlockHash &block_hash);
    void request_babe_epoch_and_block_header(
        const std::shared_ptr<boost::asio::io_context> &exec_context,
        const primitives::BlockHeader &block_header,
        const primitives::BlockHash &block_hash);
    void request_session_index_and_info(
        const primitives::BlockHash &block_hash,
        const primitives::BlockHash &parent_hash);

    template <typename Func>
    void for_ACU(const primitives::BlockHash &block_hash, Func &&func);

    static void store_included_candidates(
        ApprovingContextUnit &acu, CandidateIncludedList &&candidates_list);
    void try_process_approving_context(ApprovingContextUnit &acu);

    std::optional<std::pair<ValidatorIndex, crypto::Sr25519Keypair>>
    findAssignmentKey(std::shared_ptr<crypto::CryptoStore> const &keystore,
                      runtime::SessionInfo const &config);

    BlockImportedCandidates processImportedBlock(
        primitives::BlockNumber block_number,
        const primitives::BlockHash &block_hash,
        ImportedBlockInfo &&block_info);

    std::vector<std::pair<CandidateHash, CandidateEntry>> add_block_entry(
        primitives::BlockNumber block_number,
        const primitives::BlockHash &block_hash,
        ImportedBlockInfo &&block_info);

    void on_active_leaves_update(
        primitives::BlockNumber finalized,
        std::vector<primitives::BlockHash> const &heads);

    void handleTranche(const primitives::BlockHash &block_hash,
                       primitives::BlockNumber block_number,
                       const CandidateHash &candidate_hash);

    void scheduleTranche(primitives::BlockHash const &head,
                         BlockImportedCandidates &&candidate);

    inline auto &storedBlocks() {
      return as<StorePair<primitives::BlockNumber,
                          std::unordered_set<network::Hash>>>(store_);
    }

    inline auto &storedCandidateEntries() {
      return as<StorePair<network::Hash, CandidateEntry>>(store_);
    }

    ApprovingContextMap approving_context_map_;
    std::shared_ptr<thread::ThreadPool> int_pool_;
    std::shared_ptr<thread::ThreadPool> thread_pool_;
    std::shared_ptr<runtime::ParachainHost> parachain_host_;
    std::shared_ptr<consensus::BabeUtil> babe_util_;
    std::shared_ptr<crypto::CryptoStore> keystore_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<const blockchain::BlockTree> block_tree_;
    std::shared_ptr<boost::asio::io_context> this_context_;
    const ApprovalVotingSubsystem config_;
    std::shared_ptr<network::PeerView> peer_view_;
    network::PeerView::MyViewSubscriberPtr my_view_sub_;
    std::unique_ptr<clock::Timer> timer_;
    Store<StorePair<primitives::BlockNumber, std::unordered_set<network::Hash>>,
          StorePair<network::Hash, CandidateEntry>>
        store_;
    log::Logger logger_ =
        log::createLogger("ApprovalDistribution", "parachain");
  };

}  // namespace kagome::parachain

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain, ApprovalDistribution::Error);

#endif  // KAGOME_APPROVAL_DISTRIBUTION_HPP
