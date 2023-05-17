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
#include "parachain/availability/recovery/recovery.hpp"
#include "parachain/validator/parachain_processor.hpp"
#include "runtime/runtime_api/babe_api.hpp"
#include "runtime/runtime_api/parachain_host.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"
#include "utils/safe_object.hpp"
#include "utils/thread_pool.hpp"

namespace kagome::parachain {
  using DistributeAssignment = network::Assignment;

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
      UNUSED_SLOT_TYPE = 4,
      ENTRY_IS_NOT_FOUND = 5,
      ALREADY_APPROVING = 6,
      VALIDATOR_INDEX_OUT_OF_BOUNDS = 7,
      CORE_INDEX_OUT_OF_BOUNDS = 8,
      IS_IN_BACKING_GROUP = 9,
      SAMPLE_OUT_OF_BOUNDS = 10,
      VRF_DELAY_CORE_INDEX_MISMATCH = 11,
      VRF_VERIFY_AND_GET_TRANCHE = 12,
    };

    struct OurAssignment {
      SCALE_TIE(4);
      approval::AssignmentCert cert;
      uint32_t tranche;
      ValidatorIndex validator_index;
      bool triggered;  /// Whether the assignment has been triggered already.
    };

    /// Metadata regarding a specific tranche of assignments for a specific
    /// candidate.
    struct TrancheEntry {
      SCALE_TIE(2);

      network::DelayTranche tranche;
      // Assigned validators, and the instant we received their assignment,
      // rounded to the nearest tick.
      std::vector<std::pair<ValidatorIndex, Tick>> assignments;
    };

    struct ApprovalEntry {
      SCALE_TIE(6);
      using MaybeCert = std::optional<
          std::tuple<std::reference_wrapper<const approval::AssignmentCert>,
                     ValidatorIndex,
                     DelayTranche>>;

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

      /// Whether a validator is already assigned.
      bool is_assigned(ValidatorIndex validator_index) {
        if (validator_index < assignments.bits.size()) {
          return assignments.bits[validator_index];
        }
        return false;
      }

      /// Get the number of validators in this approval entry.
      auto n_validators() const {
        return assignments.bits.size();
      }

      // Produce a bitvec indicating the assignments of all validators up to and
      // including `tranche`.
      scale::BitVec assignments_up_to(const DelayTranche tranche) const {
        scale::BitVec out;
        out.bits.assign(assignments.bits.size(), false);
        for (const auto &e : tranches) {
          if (e.tranche > tranche) {
            break;
          }
          for (const auto &[v, _] : e.assignments) {
            BOOST_ASSERT(v < out.bits.size());
            out.bits[v] = true;
          }
        }
        return out;
      }

      // Note that our assignment is triggered. No-op if already triggered.
      MaybeCert trigger_our_assignment(network::Tick const tick_now) {
        if (!our_assignment || our_assignment->triggered) {
          return std::nullopt;
        }
        our_assignment->triggered = true;
        import_assignment(
            our_assignment->tranche, our_assignment->validator_index, tick_now);
        return std::make_tuple(
            std::reference_wrapper<const approval::AssignmentCert>(
                our_assignment->cert),
            our_assignment->validator_index,
            our_assignment->tranche);
      }

      /// Import an assignment. No-op if already assigned on the same tranche.
      void import_assignment(DelayTranche tranche,
                             ValidatorIndex validator_index,
                             Tick tick_now) {
        auto upload_tranche_and_return_pos = [&]() {
          auto it = std::lower_bound(
              tranches.begin(),
              tranches.end(),
              tranche,
              [](auto const &l, auto const &r) { return l.tranche < r; });

          if (it != tranches.end()) {
            auto const pos = (size_t)std::distance(tranches.begin(), it);
            if (it->tranche > tranche) {
              tranches.insert(it,
                              TrancheEntry{
                                  .tranche = tranche,
                                  .assignments = {},
                              });
            }
            return pos;
          }
          tranches.emplace_back(TrancheEntry{
              .tranche = tranche,
              .assignments = {},
          });
          return tranches.size() - 1ul;
        };

        const auto idx = upload_tranche_and_return_pos();
        tranches[idx].assignments.emplace_back(validator_index, tick_now);
        assignments.bits[validator_index] = true;
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

      std::optional<std::reference_wrapper<ApprovalEntry>> approval_entry(
          const network::RelayHash &relay_hash) {
        if (auto it = block_assignments.find(relay_hash);
            it != block_assignments.end()) {
          return it->second;
        }
        return std::nullopt;
      }

      /// Query whether a given validator has approved the candidate.
      bool has_approved(const ValidatorIndex validator) {
        if (validator >= approvals.bits.size()) {
          return false;
        }
        return approvals.bits[validator];
      }

      /// Return the previous approval state.
      bool mark_approval(const ValidatorIndex validator) {
        BOOST_ASSERT(validator < approvals.bits.size());
        const auto prev = has_approved(validator);
        approvals.bits[validator] = true;
        return prev;
      }

      bool operator==(const CandidateEntry &c) {
        auto block_assignments_eq = [&]() {
          if (block_assignments.size() != c.block_assignments.size()) {
            return false;
          }
          for (auto const &[h, ae] : block_assignments) {
            auto it = c.block_assignments.find(h);
            if (it == c.block_assignments.end() || it->second != ae) {
              return false;
            }
          }
          return true;
        };

        return candidate == c.candidate && session == c.session
            && approvals == c.approvals && block_assignments_eq();
      }
    };

    ApprovalDistribution(
        std::shared_ptr<runtime::BabeApi> babe_api,
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<ThreadPool> thread_pool,
        std::shared_ptr<runtime::ParachainHost> parachain_host,
        std::shared_ptr<consensus::babe::BabeUtil> babe_util,
        std::shared_ptr<crypto::CryptoStore> keystore,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<network::PeerView> peer_view,
        std::shared_ptr<ParachainProcessorImpl> parachain_processor,
        std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
        std::shared_ptr<network::PeerManager> pm,
        std::shared_ptr<network::Router> router,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<parachain::Pvf> pvf,
        std::shared_ptr<parachain::Recovery> recovery);
    ~ApprovalDistribution() = default;

    /// AppStateManager impl
    bool prepare();

    void onValidationProtocolMsg(
        const libp2p::peer::PeerId &peer_id,
        const network::ValidatorProtocolMessage &message);

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
      consensus::babe::BabeSlotNumber slot;
      runtime::SessionInfo session_info;
      std::optional<primitives::BlockNumber> force_approve;
    };

    struct ApprovingContext {
      primitives::BlockHeader block_header;
      std::optional<CandidateIncludedList> included_candidates;
      std::optional<SessionIndex> session_index;
      std::optional<consensus::babe::BabeBlockHeader> babe_block_header;
      std::optional<consensus::babe::EpochNumber> babe_epoch;
      std::optional<primitives::Randomness> randomness;
      std::optional<primitives::AuthorityList> authorities;
      std::optional<runtime::SessionInfo> session_info;

      std::shared_ptr<boost::asio::io_context> complete_callback_context;
      std::function<void(outcome::result<ImportedBlockInfo> &&)>
          complete_callback;

      bool is_complete() const {
        return included_candidates && babe_epoch && session_index
            && session_info && babe_block_header && randomness && authorities;
      }
    };

    using DistribApprovalStateAssigned = approval::AssignmentCert;
    using DistribApprovalStateApproved =
        std::pair<approval::AssignmentCert, ValidatorSignature>;
    using DistribApprovalState = boost::variant<DistribApprovalStateAssigned,
                                                DistribApprovalStateApproved>;

    // routing state bundled with messages for the candidate. Corresponding
    // assignments
    // and approvals are stored together and should be routed in the same way,
    // with assignments preceding approvals in all cases.
    struct MessageState {
      DistribApprovalState approval_state;
      bool local;
    };

    /// Information about candidates in the context of a particular block they
    /// are included in. In other words, multiple `CandidateEntry`s may exist
    /// for the same candidate, if it is included by multiple blocks - this is
    /// likely the case when there are forks.
    struct DistribCandidateEntry {
      std::unordered_map<ValidatorIndex, MessageState> messages;
    };

    /// Information about blocks in our current view as well as whether peers
    /// know of them.
    struct DistribBlockEntry {
      /// A votes entry for each candidate indexed by [`CandidateIndex`].
      std::vector<DistribCandidateEntry> candidates;
    };

    /// Metadata regarding approval of a particular block, by way of approval of
    /// the candidates contained within it.
    struct BlockEntry {
      primitives::BlockHash block_hash;
      primitives::BlockHash parent_hash;
      primitives::BlockNumber block_number;
      SessionIndex session;
      runtime::SessionInfo session_info;
      consensus::babe::BabeSlotNumber slot;
      RelayVRFStory relay_vrf_story;
      // The candidates included as-of this block and the index of the core they
      // are leaving. Sorted ascending by core index.
      std::vector<std::pair<CoreIndex, CandidateHash>> candidates;
      // A bitfield where the i'th bit corresponds to the i'th candidate in
      // `candidates`. The i'th bit is `true` iff the candidate has been
      // approved in the context of this block. The block can be considered
      // approved if the bitfield has all bits set to `true`.
      scale::BitVec approved_bitfield;
      std::vector<Hash> children;

      std::optional<CandidateIndex> candidateIxByHash(
          const CandidateHash &candidate_hash) {
        for (size_t ix = 0ul; ix < candidates.size(); ++ix) {
          const auto &[_, h] = candidates[ix];
          if (h == candidate_hash) {
            return CandidateIndex(ix);
          }
        }
        return std::nullopt;
      }

      /// Mark a candidate as fully approved in the bitfield.
      void mark_approved_by_hash(const CandidateHash &candidate_hash) {
        if (auto p = candidateIxByHash(candidate_hash)) {
          approved_bitfield.bits[*p] = true;
        }
      }

      /// Whether the block entry is fully approved.
      bool is_fully_approved() const {
        return approval::count_ones(approved_bitfield)
            == approved_bitfield.bits.size();
      }

      /// Whether a candidate is approved in the bitfield.
      bool is_candidate_approved(const CandidateHash &candidate_hash) {
        if (auto pos = candidateIxByHash(candidate_hash);
            pos && *pos < approved_bitfield.bits.size()) {
          return approved_bitfield.bits[*pos];
        }
        return false;
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

    using AssignmentOrApproval =
        boost::variant<network::Assignment,
                       network::IndirectSignedApprovalVote>;
    using PendingMessage = AssignmentOrApproval;

    using MessageSource =
        std::optional<std::reference_wrapper<const libp2p::peer::PeerId>>;

    enum ApprovalOutcome {
      Approved,
      Failed,
      TimedOut,
    };

    /// The result type of [`ApprovalVotingMessage::CheckAndImportAssignment`]
    /// request.
    enum struct AssignmentCheckResult {
      /// The vote was accepted and should be propagated onwards.
      Accepted,
      /// The vote was valid but duplicate and should not be propagated onwards.
      AcceptedDuplicate,
      /// The vote was valid but too far in the future to accept right now.
      TooFarInFuture,
      /// The vote was bad and should be ignored, reporting the peer who
      /// propagated it.
      Bad
    };

    /// The result type of [`ApprovalVotingMessage::CheckAndImportApproval`]
    /// request.
    enum struct ApprovalCheckResult {
      /// The vote was accepted and should be propagated onwards.
      Accepted,
      /// The vote was bad and should be ignored, reporting the peer who
      /// propagated it.
      Bad
    };

    using ApprovingContextMap =
        std::unordered_map<primitives::BlockHash, ApprovingContext>;
    using ApprovingContextUnit = ApprovingContextMap::iterator::value_type;
    using NewHeadDataContext =
        std::tuple<ApprovalDistribution::CandidateIncludedList,
                   std::pair<SessionIndex, runtime::SessionInfo>,
                   std::tuple<consensus::babe::EpochNumber,
                              consensus::babe::BabeBlockHeader,
                              primitives::AuthorityList,
                              primitives::Randomness>>;

    AssignmentsList compute_assignments(
        const std::shared_ptr<crypto::CryptoStore> &keystore,
        const runtime::SessionInfo &config,
        const RelayVRFStory &relay_vrf_story,
        const CandidateIncludedList &leaving_cores);

    void imported_block_info(const primitives::BlockHash &block_hash,
                             const primitives::BlockHeader &block_header);

    ApprovalOutcome validate_candidate_exhaustive(
        const runtime::PersistedValidationData &data,
        const network::ParachainBlock &pov,
        const network::CandidateReceipt &receipt,
        const ParachainRuntime &code);

    AssignmentCheckResult check_and_import_assignment(
        const approval::IndirectAssignmentCert &assignment,
        CandidateIndex claimed_candidate_index);
    ApprovalCheckResult check_and_import_approval(
        const network::IndirectSignedApprovalVote &vote);
    void import_and_circulate_assignment(
        const MessageSource &source,
        const approval::IndirectAssignmentCert &assignment,
        CandidateIndex claimed_candidate_index);
    void import_and_circulate_approval(
        const MessageSource &source,
        const network::IndirectSignedApprovalVote &vote);

    template <typename Func>
    void handle_new_head(const primitives::BlockHash &head,
                         const network::ExView &updated,
                         Func &&func);
    std::optional<std::pair<std::reference_wrapper<ApprovalEntry>,
                            approval::ApprovalStatus>>
    approval_status(const BlockEntry &block_entry,
                    CandidateEntry &candidate_entry);

    outcome::result<ApprovalDistribution::CandidateIncludedList>
    request_included_candidates(const primitives::BlockHash &block_hash);
    outcome::result<std::tuple<consensus::babe::EpochNumber,
                               consensus::babe::BabeBlockHeader,
                               primitives::AuthorityList,
                               primitives::Randomness>>
    request_babe_epoch_and_block_header(
        const primitives::BlockHeader &block_header,
        const primitives::BlockHash &block_hash);
    outcome::result<std::pair<SessionIndex, runtime::SessionInfo>>
    request_session_index_and_info(const primitives::BlockHash &block_hash,
                                   const primitives::BlockHash &parent_hash);

    template <typename Func>
    void for_ACU(const primitives::BlockHash &block_hash, Func &&func);

    void try_process_approving_context(ApprovingContextUnit &acu);

    std::optional<std::pair<ValidatorIndex, crypto::Sr25519Keypair>>
    findAssignmentKey(const std::shared_ptr<crypto::CryptoStore> &keystore,
                      const runtime::SessionInfo &config);

    outcome::result<BlockImportedCandidates> processImportedBlock(
        primitives::BlockNumber block_number,
        const primitives::BlockHash &block_hash,
        const primitives::BlockHash &parent_hash,
        ImportedBlockInfo &&block_info);

    outcome::result<std::vector<std::pair<CandidateHash, CandidateEntry>>>
    add_block_entry(primitives::BlockNumber block_number,
                    const primitives::BlockHash &block_hash,
                    const primitives::BlockHash &parent_hash,
                    scale::BitVec &&approved_bitfield,
                    ImportedBlockInfo &&block_info);

    void on_active_leaves_update(const network::ExView &updated);

    void handleTranche(const primitives::BlockHash &block_hash,
                       primitives::BlockNumber block_number,
                       const CandidateHash &candidate_hash);

    void launch_approval(const RelayHash &relay_block_hash,
                         const CandidateHash &candidate_hash,
                         SessionIndex session_index,
                         const network::CandidateReceipt &candidate,
                         ValidatorIndex validator_index,
                         Hash block_hash,
                         GroupIndex backing_group);

    void issue_approval(const CandidateHash &candidate_hash,
                        ValidatorIndex validator_index,
                        const RelayHash &block_hash);

    void runLaunchApproval(
        const CandidateHash &candidate_hash,
        const approval::IndirectAssignmentCert &indirect_cert,
        DelayTranche assignment_tranche,
        const RelayHash &relay_block_hash,
        CandidateIndex candidate_index,
        SessionIndex session,
        const network::CandidateReceipt &candidate,
        GroupIndex backing_group);

    void runNewBlocks(approval::BlockApprovalMeta &&approval_meta);

    std::optional<ValidatorSignature> sign_approval(
        const crypto::Sr25519PublicKey &pubkey,
        SessionIndex session_index,
        const CandidateHash &candidate_hash);

    void storeNewHeadContext(const primitives::BlockHash &block_hash,
                             NewHeadDataContext &&ctx);

    void notifyApproved(const Hash &block_hash);

    void advance_approval_state(BlockEntry &block_entry,
                                const CandidateHash &candidate_hash,
                                CandidateEntry &candidate_entry,
                                approval::ApprovalStateTransition transition);

    void schedule_wakeup_action(
        const ApprovalEntry &approval_entry,
        const Hash &block_hash,
        BlockNumber block_number,
        const CandidateHash &candidate_hash,
        Tick block_tick,
        Tick tick_now,
        const approval::RequiredTranches &required_tranches);

    void scheduleTranche(const primitives::BlockHash &head,
                         BlockImportedCandidates &&candidate);

    void runDistributeAssignment(
        const approval::IndirectAssignmentCert &indirect_cert,
        CandidateIndex candidate_index);

    void runDistributeApproval(const network::IndirectSignedApprovalVote &vote);

    void runScheduleWakeup(const primitives::BlockHash &block_hash,
                           primitives::BlockNumber block_number,
                           const CandidateHash &candidate_hash,
                           Tick tick);

    void clearCaches(const primitives::events::ChainEventParams &event);

    auto &storedBlocks() {
      return as<StorePair<primitives::BlockNumber,
                          std::unordered_set<network::Hash>>>(store_);
    }

    auto &storedCandidateEntries() {
      return as<StorePair<network::Hash, CandidateEntry>>(store_);
    }

    auto &storedBlockEntries() {
      return as<StorePair<RelayHash, BlockEntry>>(store_);
    }

    auto &storedDistribBlockEntries() {
      return as<StorePair<Hash, DistribBlockEntry>>(store_);
    }

    ApprovingContextMap approving_context_map_;
    std::shared_ptr<ThreadPool> int_pool_;
    std::shared_ptr<ThreadHandler> internal_context_;

    std::shared_ptr<ThreadPool> thread_pool_;
    std::shared_ptr<ThreadHandler> thread_pool_context_;

    std::shared_ptr<runtime::ParachainHost> parachain_host_;
    std::shared_ptr<consensus::babe::BabeUtil> babe_util_;
    std::shared_ptr<crypto::CryptoStore> keystore_;
    std::shared_ptr<crypto::Hasher> hasher_;
    const ApprovalVotingSubsystem config_;
    std::shared_ptr<network::PeerView> peer_view_;
    network::PeerView::MyViewSubscriberPtr my_view_sub_;
    std::shared_ptr<primitives::events::ChainEventSubscriber> chain_sub_;

    Store<StorePair<primitives::BlockNumber, std::unordered_set<Hash>>,
          StorePair<CandidateHash, CandidateEntry>,
          StorePair<RelayHash, BlockEntry>,
          StorePair<Hash, DistribBlockEntry>>
        store_;

    std::shared_ptr<ParachainProcessorImpl> parachain_processor_;
    std::shared_ptr<crypto::Sr25519Provider> crypto_provider_;
    std::shared_ptr<network::PeerManager> pm_;
    std::shared_ptr<network::Router> router_;
    std::shared_ptr<runtime::BabeApi> babe_api_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<parachain::Pvf> pvf_;
    std::shared_ptr<parachain::Recovery> recovery_;
    std::unordered_map<
        Hash,
        std::vector<std::pair<libp2p::peer::PeerId, PendingMessage>>>
        pending_known_;

    /// thread_pool_ context access
    using ScheduledCandidateTimer =
        std::unordered_map<CandidateHash,
                           std::pair<Tick, std::unique_ptr<clock::Timer>>>;
    std::unordered_map<network::BlockHash, ScheduledCandidateTimer>
        active_tranches_;

    log::Logger logger_ =
        log::createLogger("ApprovalDistribution", "parachain");
  };

}  // namespace kagome::parachain

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain, ApprovalDistribution::Error);

#endif  // KAGOME_APPROVAL_DISTRIBUTION_HPP
