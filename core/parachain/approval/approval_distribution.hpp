/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_set>
#include <vector>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/variant.hpp>
#include <libp2p/basic/scheduler.hpp>

#include "blockchain/block_tree.hpp"
#include "consensus/babe/types/babe_block_header.hpp"
#include "consensus/timeline/slots_util.hpp"
#include "consensus/timeline/types.hpp"
#include "crypto/key_store/key_file_storage.hpp"
#include "crypto/key_store/session_keys.hpp"
#include "crypto/type_hasher.hpp"
#include "dispute_coordinator/dispute_coordinator.hpp"
#include "injector/lazy.hpp"
#include "network/peer_view.hpp"
#include "network/types/collator_messages_vstaging.hpp"
#include "parachain/approval/approved_ancestor.hpp"
#include "parachain/approval/knowledge.hpp"
#include "parachain/approval/store.hpp"
#include "parachain/availability/recovery/recovery.hpp"
#include "parachain/validator/parachain_processor.hpp"
#include "runtime/runtime_api/parachain_host.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"
#include "utils/safe_object.hpp"

namespace kagome {
  class PoolHandler;
  class PoolHandlerReady;
}  // namespace kagome

namespace kagome::common {
  class MainThreadPool;
  class WorkerThreadPool;
}  // namespace kagome::common

namespace kagome::consensus::babe {
  class BabeConfigRepository;
}

namespace kagome::parachain {
  class ApprovalThreadPool;
}

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
      : public std::enable_shared_from_this<ApprovalDistribution>,
        public IApprovedAncestor {
    struct OurAssignment {
      SCALE_TIE(4);
      approval::AssignmentCert cert;
      uint32_t tranche;
      ValidatorIndex validator_index;
      bool triggered;  /// Whether the assignment has been triggered already.
    };

    using HashedCandidateReceipt = crypto::
        Hashed<network::CandidateReceipt, 32, crypto::Blake2b_StreamHasher<32>>;

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
          std::optional<std::reference_wrapper<const OurAssignment>> assignment,
          size_t assignments_size)
          : backing_group{group_index},
            our_assignment{assignment},
            approved(false) {
        assignments.bits.insert(
            assignments.bits.end(), assignments_size, false);
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
      MaybeCert trigger_our_assignment(const network::Tick tick_now) {
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
      HashedCandidateReceipt candidate;
      SessionIndex session;
      // Assignments are based on blocks, so we need to track assignments
      // separately based on the block we are looking at.
      std::unordered_map<network::Hash, ApprovalEntry> block_assignments;
      scale::BitVec approvals;

      CandidateEntry(const HashedCandidateReceipt &hashed_receipt,
                     SessionIndex session_index,
                     size_t approvals_size)
          : candidate(hashed_receipt), session(session_index) {
        approvals.bits.insert(approvals.bits.end(), approvals_size, false);
      }

      CandidateEntry(const network::CandidateReceipt &receipt,
                     SessionIndex session_index,
                     size_t approvals_size)
          : CandidateEntry(
              HashedCandidateReceipt{receipt}, session_index, approvals_size) {}

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

      bool operator==(const CandidateEntry &c) const {
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

        return candidate.getHash() == c.candidate.getHash()
            && session == c.session && approvals == c.approvals
            && block_assignments_eq();
      }
    };

    ApprovalDistribution(
        std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo,
        std::shared_ptr<application::AppStateManager> app_state_manager,
        primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
        common::WorkerThreadPool &worker_thread_pool,
        std::shared_ptr<runtime::ParachainHost> parachain_host,
        LazySPtr<consensus::SlotsUtil> slots_util,
        std::shared_ptr<crypto::KeyStore> keystore,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<network::PeerView> peer_view,
        std::shared_ptr<ParachainProcessorImpl> parachain_processor,
        std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
        std::shared_ptr<network::PeerManager> pm,
        std::shared_ptr<network::Router> router,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<parachain::Pvf> pvf,
        std::shared_ptr<parachain::Recovery> recovery,
        ApprovalThreadPool &approval_thread_pool,
        common::MainThreadPool &main_thread_pool,
        LazySPtr<dispute::DisputeCoordinator> dispute_coordinator);
    ~ApprovalDistribution() = default;

    /// AppStateManager impl
    bool tryStart();

    using CandidateIncludedList =
        std::vector<std::tuple<HashedCandidateReceipt, CoreIndex, GroupIndex>>;
    using AssignmentsList = std::unordered_map<CoreIndex, OurAssignment>;

    static AssignmentsList compute_assignments(
        const std::shared_ptr<crypto::KeyStore> &keystore,
        const runtime::SessionInfo &config,
        const RelayVRFStory &relay_vrf_story,
        const CandidateIncludedList &leaving_cores);

    void onValidationProtocolMsg(
        const libp2p::peer::PeerId &peer_id,
        const network::VersionedValidatorProtocolMessage &message);

    using SignaturesForCandidate =
        std::unordered_map<ValidatorIndex, ValidatorSignature>;
    using SignaturesForCandidateCallback =
        std::function<void(SignaturesForCandidate &&)>;

    void getApprovalSignaturesForCandidate(
        const CandidateHash &candidate,
        SignaturesForCandidateCallback &&callback);

    primitives::BlockInfo approvedAncestor(
        const primitives::BlockInfo &min,
        const primitives::BlockInfo &max) const override;

   private:
    struct ImportedBlockInfo {
      CandidateIncludedList included_candidates;
      SessionIndex session_index;
      AssignmentsList assignments;
      size_t n_validators;
      RelayVRFStory relay_vrf_story;
      consensus::SlotNumber slot;
      std::optional<primitives::BlockNumber> force_approve;
    };

    struct ApprovingContext {
      primitives::BlockHeader block_header;
      std::optional<CandidateIncludedList> included_candidates;
      std::optional<consensus::babe::BabeBlockHeader> babe_block_header;
      std::optional<consensus::EpochNumber> babe_epoch;
      std::optional<consensus::Randomness> randomness;
      std::optional<consensus::babe::Authorities> authorities;

      std::function<void(outcome::result<ImportedBlockInfo> &&)>
          complete_callback;

      bool is_complete() const {
        return included_candidates && babe_epoch && babe_block_header
            && randomness && authorities;
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
      std::unordered_map<ValidatorIndex, MessageState> messages{};
    };

    /// Information about blocks in our current view as well as whether peers
    /// know of them.
    struct DistribBlockEntry {
      /// A votes entry for each candidate indexed by [`CandidateIndex`].
      std::vector<DistribCandidateEntry> candidates{};
      /// Our knowledge of messages.
      approval::Knowledge knowledge{};
      /// Peers who we know are aware of this block and thus, the candidates
      /// within it. This maps to their knowledge of messages.
      std::unordered_map<libp2p::peer::PeerId, approval::PeerKnowledge>
          known_by{};
      /// The number of the block.
      primitives::BlockNumber number;
      /// The parent hash of the block.
      RelayHash parent_hash;
    };

    /// Metadata regarding approval of a particular block, by way of approval of
    /// the candidates contained within it.
    struct BlockEntry {
      primitives::BlockHash block_hash;
      primitives::BlockHash parent_hash;
      primitives::BlockNumber block_number;
      SessionIndex session;
      consensus::SlotNumber slot;
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
      primitives::BlockHash block_hash{};
      primitives::BlockNumber block_number{};
      network::Tick block_tick{};
      network::Tick no_show_duration{};
      std::vector<std::pair<CandidateHash, CandidateEntry>>
          imported_candidates{};
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
                   std::tuple<consensus::EpochNumber,
                              consensus::babe::BabeBlockHeader,
                              consensus::babe::Authorities,
                              consensus::Randomness>>;

    void imported_block_info(const primitives::BlockHash &block_hash,
                             const primitives::BlockHeader &block_header);

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
    outcome::result<std::tuple<consensus::EpochNumber,
                               consensus::babe::BabeBlockHeader,
                               consensus::babe::Authorities,
                               consensus::Randomness>>
    request_babe_epoch_and_block_header(
        const primitives::BlockHeader &block_header,
        const primitives::BlockHash &block_hash);
    outcome::result<std::pair<SessionIndex, runtime::SessionInfo>>
    request_session_index_and_info(const primitives::BlockHash &block_hash,
                                   const primitives::BlockHash &parent_hash);

    template <typename Func>
    void for_ACU(const primitives::BlockHash &block_hash, Func &&func);

    void try_process_approving_context(
        ApprovingContextUnit &acu,
        SessionIndex session_index,
        const runtime::SessionInfo &session_info);

    static std::optional<std::pair<ValidatorIndex, crypto::Sr25519Keypair>>
    findAssignmentKey(const std::shared_ptr<crypto::KeyStore> &keystore,
                      const runtime::SessionInfo &config);

    void unify_with_peer(StoreUnit<StorePair<Hash, DistribBlockEntry>> &entries,
                         const libp2p::peer::PeerId &peer_id,
                         const network::View &view);

    outcome::result<BlockImportedCandidates> processImportedBlock(
        primitives::BlockNumber block_number,
        const primitives::BlockHash &block_hash,
        const primitives::BlockHash &parent_hash,
        primitives::BlockNumber finalized_block_number,
        ImportedBlockInfo &&block_info);

    outcome::result<std::vector<std::pair<CandidateHash, CandidateEntry>>>
    add_block_entry(primitives::BlockNumber block_number,
                    const primitives::BlockHash &block_hash,
                    const primitives::BlockHash &parent_hash,
                    scale::BitVec &&approved_bitfield,
                    const ImportedBlockInfo &block_info);

    void on_active_leaves_update(const network::ExView &updated);

    void handleTranche(const primitives::BlockHash &block_hash,
                       primitives::BlockNumber block_number,
                       const CandidateHash &candidate_hash);

    void launch_approval(const RelayHash &relay_block_hash,
                         SessionIndex session_index,
                         const HashedCandidateReceipt &hashed_receipt,
                         ValidatorIndex validator_index,
                         Hash block_hash,
                         GroupIndex backing_group);

    void issue_approval(const CandidateHash &candidate_hash,
                        ValidatorIndex validator_index,
                        const RelayHash &block_hash);

    void runLaunchApproval(
        const approval::IndirectAssignmentCert &indirect_cert,
        DelayTranche assignment_tranche,
        const RelayHash &relay_block_hash,
        CandidateIndex candidate_index,
        SessionIndex session,
        const HashedCandidateReceipt &hashed_candidate,
        GroupIndex backing_group);

    void runNewBlocks(approval::BlockApprovalMeta &&approval_meta,
                      primitives::BlockNumber finalized_block_number);

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
        CandidateIndex candidate_index,
        std::unordered_set<libp2p::peer::PeerId> &&peers);

    void send_assignments_batched(std::deque<network::Assignment> &&assignments,
                                  const libp2p::peer::PeerId &peer_id);

    void send_approvals_batched(
        std::deque<network::IndirectSignedApprovalVote> &&approvals,
        const libp2p::peer::PeerId &peer_id);

    void runDistributeApproval(
        const network::IndirectSignedApprovalVote &vote,
        std::unordered_set<libp2p::peer::PeerId> &&peers);

    void runScheduleWakeup(const primitives::BlockHash &block_hash,
                           primitives::BlockNumber block_number,
                           const CandidateHash &candidate_hash,
                           Tick tick);

    void clearCaches(
        const primitives::events::RemoveAfterFinalizationParams &event);

    void store_remote_view(const libp2p::peer::PeerId &peer_id,
                           const network::View &view);

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

    auto &storedBlockEntries() const {
      return as<StorePair<RelayHash, BlockEntry>>(store_);
    }

    auto &storedDistribBlockEntries() {
      return as<StorePair<Hash, DistribBlockEntry>>(store_);
    }

    log::Logger logger_ =
        log::createLogger("ApprovalDistribution", "parachain");

    ApprovingContextMap approving_context_map_;
    std::shared_ptr<PoolHandlerReady> approval_thread_handler_;

    std::shared_ptr<PoolHandler> worker_pool_handler_;

    std::shared_ptr<runtime::ParachainHost> parachain_host_;
    LazySPtr<consensus::SlotsUtil> slots_util_;
    std::shared_ptr<crypto::KeyStore> keystore_;
    std::shared_ptr<crypto::Hasher> hasher_;
    const ApprovalVotingSubsystem config_;
    std::shared_ptr<network::PeerView> peer_view_;
    network::PeerView::MyViewSubscriberPtr my_view_sub_;
    network::PeerView::PeerViewSubscriberPtr remote_view_sub_;
    primitives::events::ChainSub chain_sub_;

    Store<StorePair<primitives::BlockNumber, std::unordered_set<Hash>>,
          StorePair<CandidateHash, CandidateEntry>,
          StorePair<RelayHash, BlockEntry>,
          StorePair<Hash, DistribBlockEntry>>
        store_;

    std::shared_ptr<ParachainProcessorImpl> parachain_processor_;
    std::shared_ptr<crypto::Sr25519Provider> crypto_provider_;
    std::shared_ptr<network::PeerManager> pm_;
    std::shared_ptr<network::Router> router_;
    std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<parachain::Pvf> pvf_;
    std::shared_ptr<parachain::Recovery> recovery_;
    std::shared_ptr<PoolHandler> main_pool_handler_;
    LazySPtr<dispute::DisputeCoordinator> dispute_coordinator_;

    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;

    std::unordered_map<
        Hash,
        std::vector<std::pair<libp2p::peer::PeerId, PendingMessage>>>
        pending_known_;
    std::unordered_map<libp2p::peer::PeerId, network::View> peer_views_;
    std::map<primitives::BlockNumber, std::unordered_set<primitives::BlockHash>>
        blocks_by_number_;

    using ScheduledCandidateTimer = std::unordered_map<
        CandidateHash,
        std::vector<std::pair<Tick, libp2p::basic::Scheduler::Handle>>>;
    std::unordered_map<network::BlockHash, ScheduledCandidateTimer>
        active_tranches_;

    struct ApprovalCache {
      std::unordered_set<primitives::BlockHash> blocks_;
      ApprovalOutcome approval_result;
    };
    SafeObject<std::unordered_map<CandidateHash, ApprovalCache>, std::mutex>
        approvals_cache_;
  };

}  // namespace kagome::parachain
