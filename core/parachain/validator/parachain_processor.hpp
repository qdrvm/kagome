/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <queue>
#include <random>
#include <thread>
#include <unordered_map>

#include <libp2p/peer/peer_id.hpp>

#include "application/app_configuration.hpp"
#include "authority_discovery/query/query.hpp"
#include "common/visitor.hpp"
#include "crypto/hasher.hpp"
#include "metrics/metrics.hpp"
#include "network/peer_manager.hpp"
#include "network/peer_view.hpp"
#include "network/protocols/req_collation_protocol.hpp"
#include "network/types/collator_messages_vstaging.hpp"
#include "outcome/outcome.hpp"
#include "parachain/availability/bitfield/signer.hpp"
#include "parachain/availability/store/store.hpp"
#include "parachain/backing/store.hpp"
#include "parachain/pvf/precheck.hpp"
#include "parachain/pvf/pvf.hpp"
#include "parachain/validator/backing_implicit_view.hpp"
#include "parachain/validator/collations.hpp"
#include "parachain/validator/impl/candidates.hpp"
#include "parachain/validator/impl/statements_store.hpp"
#include "parachain/validator/prospective_parachains.hpp"
#include "parachain/validator/signer.hpp"
#include "primitives/common.hpp"
#include "primitives/event_types.hpp"
#include "utils/non_copyable.hpp"
#include "utils/safe_object.hpp"
#include "utils/weak_io_context.hpp"

namespace kagome {
  class ThreadHandler;
}

namespace kagome::common {
  class WorkerThreadPool;
}

namespace kagome::network {
  class Router;
}  // namespace kagome::network

namespace kagome::crypto {
  class Sr25519Provider;
  class Hasher;
  class SessionKeys;
}  // namespace kagome::crypto

namespace kagome::dispute {
  class RuntimeInfo;
}

namespace kagome::parachain {

  struct BackedCandidatesSource {
    virtual ~BackedCandidatesSource() {}
    virtual std::vector<network::BackedCandidate> getBackedCandidates(
        const RelayHash &relay_parent) = 0;
  };

  struct ParachainProcessorImpl
      : BackedCandidatesSource,
        std::enable_shared_from_this<ParachainProcessorImpl> {
    enum class Error {
      RESPONSE_ALREADY_RECEIVED = 1,
      COLLATION_NOT_FOUND,
      KEY_NOT_PRESENT,
      VALIDATION_FAILED,
      VALIDATION_SKIPPED,
      OUT_OF_VIEW,
      DUPLICATE,
      NO_INSTANCE,
      NOT_A_VALIDATOR,
      NOT_SYNCHRONIZED,
      UNDECLARED_COLLATOR,
      PEER_LIMIT_REACHED,
      PROTOCOL_MISMATCH,
      NOT_CONFIRMED,
      NO_STATE,
      NO_SESSION_INFO,
      OUT_OF_BOUND,
      REJECTED_BY_PROSPECTIVE_PARACHAINS
    };
    static constexpr uint64_t kBackgroundWorkers = 5;

    struct ImportStatementSummary {
      BackingStore::ImportResult imported;
      /// Attested more than threshold
      bool attested;
    };

    enum struct ValidationTaskType { kSecond, kAttest };

    ParachainProcessorImpl(
        std::shared_ptr<network::PeerManager> pm,
        std::shared_ptr<dispute::RuntimeInfo> runtime_info,
        std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
        std::shared_ptr<network::Router> router,
        WeakIoContext main_thread_context,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<network::PeerView> peer_view,
        std::shared_ptr<common::WorkerThreadPool> worker_thread_pool,
        std::shared_ptr<parachain::BitfieldSigner> bitfield_signer,
        std::shared_ptr<parachain::PvfPrecheck> pvf_precheck,
        std::shared_ptr<parachain::BitfieldStore> bitfield_store,
        std::shared_ptr<parachain::BackingStore> backing_store,
        std::shared_ptr<parachain::Pvf> pvf,
        std::shared_ptr<parachain::AvailabilityStore> av_store,
        std::shared_ptr<runtime::ParachainHost> parachain_host,
        std::shared_ptr<parachain::ValidatorSignerFactory> signer_factory,
        const application::AppConfiguration &app_config,
        std::shared_ptr<application::AppStateManager> app_state_manager,
        primitives::events::BabeStateSubscriptionEnginePtr
            babe_status_observable,
        std::shared_ptr<authority_discovery::Query> query_audi,
        std::shared_ptr<ProspectiveParachains> prospective_parachains);
    ~ParachainProcessorImpl() = default;

    bool prepare();
    bool start();

    void handleAdvertisement(
        network::CollationEvent &&pending_collation,
        std::optional<std::pair<CandidateHash, Hash>> &&prospective_candidate);
    outcome::result<void> canProcessParachains() const;

    void onIncomingCollator(const libp2p::peer::PeerId &peer_id,
                            network::CollatorPublicKey pubkey,
                            network::ParachainId para_id);
    void onIncomingCollationStream(const libp2p::peer::PeerId &peer_id,
                                   network::CollationVersion version);
    void onIncomingValidationStream(const libp2p::peer::PeerId &peer_id,
                                    network::CollationVersion version);
    void onValidationProtocolMsg(
        const libp2p::peer::PeerId &peer_id,
        const network::VersionedValidatorProtocolMessage &message);
    outcome::result<network::FetchChunkResponse> OnFetchChunkRequest(
        const network::FetchChunkRequest &request);
    outcome::result<network::vstaging::AttestedCandidateResponse>
    OnFetchAttestedCandidateRequest(
        const network::vstaging::AttestedCandidateRequest &request);

    std::vector<network::BackedCandidate> getBackedCandidates(
        const RelayHash &relay_parent) override;
    network::ResponsePov getPov(CandidateHash &&candidate_hash);
    auto getAvStore() {
      return av_store_;
    }
    auto getBackingStore() {
      return backing_store_;
    }

   private:
    enum struct StatementType { kSeconded = 0, kValid };
    using Commitments = std::shared_ptr<network::CandidateCommitments>;
    using WorkersContext = boost::asio::io_context;
    using WorkGuard = boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>;
    using SecondingAllowed = std::optional<fragment::FragmentTreeMembership>;

    struct ValidateAndSecondResult {
      outcome::result<void> result;
      primitives::BlockHash relay_parent;
      Commitments commitments;
      network::CandidateReceipt candidate;
      network::ParachainBlock pov;
      runtime::PersistedValidationData pvd;
    };

    struct AttestingData {
      network::CandidateReceipt candidate;
      primitives::BlockHash pov_hash;
      network::ValidatorIndex from_validator;
      std::queue<network::ValidatorIndex> backing;
    };

    struct TableContext {
      std::optional<ValidatorSigner> validator;
      std::unordered_map<ParachainId, std::vector<ValidatorIndex>> groups;
      std::vector<ValidatorId> validators;

      size_t minimum_votes(size_t n_validators) const {
        return std::min(size_t(2ull), n_validators);
      }
    };

    struct AttestedCandidate {
      /// The group ID that the candidate is in.
      GroupIndex group_id;
      /// The candidate data.
      network::CommittedCandidateReceipt candidate;
      /// Validity attestations.
      std::vector<std::pair<ValidatorIndex, network::ValidityAttestation>>
          validity_votes;
    };

    struct StatementWithPVDSeconded {
      network::CommittedCandidateReceipt committed_receipt;
      runtime::PersistedValidationData pvd;
    };

    struct StatementWithPVDValid {
      CandidateHash candidate_hash;
    };

    using StatementWithPVD =
        boost::variant<StatementWithPVDSeconded, StatementWithPVDValid>;

    using SignedFullStatementWithPVD = IndexedAndSigned<StatementWithPVD>;
    IndexedAndSigned<network::vstaging::CompactStatement> signed_to_compact(
        const SignedFullStatementWithPVD &s) const {
      const Hash h = candidateHashFrom(getPayload(s));
      return {
          .payload =
              {
                  .payload = visit_in_place(
                      getPayload(s),
                      [&](const StatementWithPVDSeconded &)
                          -> network::vstaging::CompactStatement {
                        return network::vstaging::SecondedCandidateHash{
                            .hash = h,
                        };
                      },
                      [&](const StatementWithPVDValid &)
                          -> network::vstaging::CompactStatement {
                        return network::vstaging::ValidCandidateHash{
                            .hash = h,
                        };
                      }),
                  .ix = s.payload.ix,
              },
          .signature = s.signature,
      };
    }

    struct RelayParentState {
      ProspectiveParachainsModeOpt prospective_parachains_mode;
      std::optional<network::ParachainId> assignment;
      std::optional<primitives::BlockHash> seconded;
      std::optional<network::ValidatorIndex> our_index;
      std::optional<network::CollatorPublicKey> required_collator;

      Collations collations;
      TableContext table_context;
      std::optional<StatementStore> statement_store;
      std::vector<runtime::CoreState> availability_cores;
      runtime::GroupDescriptor group_rotation_info;
      uint32_t minimum_backing_votes;

      std::unordered_set<primitives::BlockHash> awaiting_validation;
      std::unordered_set<primitives::BlockHash> issued_statements;
      std::unordered_set<network::PeerId> peers_advertised;
      std::unordered_map<primitives::BlockHash, AttestingData> fallbacks;
      std::unordered_set<CandidateHash> backed_hashes{};
    };

    struct PerCandidateState {
      runtime::PersistedValidationData persisted_validation_data;
      bool seconded_locally;
      ParachainId para_id;
      Hash relay_parent;
    };

    struct ManifestSummary {
      Hash claimed_parent_hash;
      GroupIndex claimed_group_index;
      network::vstaging::StatementFilter statement_knowledge;
    };

    struct ManifestImportSuccess {
      bool acknowledge;
      ValidatorIndex sender_index;
    };
    using ManifestImportSuccessOpt = std::optional<ManifestImportSuccess>;

    /*
     * Validation.
     */
    outcome::result<void> advCanBeProcessed(
        const primitives::BlockHash &relay_parent,
        const libp2p::peer::PeerId &peer_id);
    outcome::result<Pvf::Result> validateCandidate(
        const network::CandidateReceipt &candidate,
        const network::ParachainBlock &pov,
        runtime::PersistedValidationData &&pvd);

    outcome::result<std::vector<network::ErasureChunk>> validateErasureCoding(
        const runtime::AvailableData &validating_data, size_t n_validators);

    template <ParachainProcessorImpl::ValidationTaskType kMode>
    void validateAsync(network::CandidateReceipt &&candidate,
                       network::ParachainBlock &&pov,
                       runtime::PersistedValidationData &&pvd,
                       const libp2p::peer::PeerId &peer_id,
                       const primitives::BlockHash &relay_parent,
                       size_t n_validators);

    template <ParachainProcessorImpl::ValidationTaskType kMode>
    void makeAvailable(const libp2p::peer::PeerId &peer_id,
                       const primitives::BlockHash &candidate_hash,
                       ValidateAndSecondResult &&result);
    void handleStatement(const primitives::BlockHash &relay_parent,
                         const SignedFullStatementWithPVD &statement);
    void process_bitfield_distribution(
        const network::BitfieldDistributionMessage &val);
    void process_legacy_statement(
        const libp2p::peer::PeerId &peer_id,
        const network::StatementDistributionMessage &msg);
    void process_vstaging_statement(
        const libp2p::peer::PeerId &peer_id,
        const network::vstaging::StatementDistributionMessage &msg);
    void send_backing_fresh_statements(
        const ConfirmedCandidate &confirmed,
        const RelayHash &relay_parent,
        ParachainProcessorImpl::RelayParentState &per_relay_parent,
        const std::vector<ValidatorIndex> &group,
        const CandidateHash &candidate_hash);
    void apply_post_confirmation(const PostConfirmation &post_confirmation);
    std::optional<GroupIndex> group_for_para(
        const std::vector<runtime::CoreState> &availability_cores,
        const runtime::GroupDescriptor &group_rotation_info,
        ParachainId para_id) const;
    void new_confirmed_candidate_fragment_tree_updates(
        const HypotheticalCandidate &candidate);
    void new_leaf_fragment_tree_updates(const Hash &leaf_hash);
    void prospective_backed_notification_fragment_tree_updates(
        ParachainId para_id, const Hash &para_head);
    void fragment_tree_update_inner(
        std::optional<std::reference_wrapper<const Hash>> active_leaf_hash,
        std::optional<std::pair<std::reference_wrapper<const Hash>,
                                ParachainId>> required_parent_info,
        std::optional<std::reference_wrapper<const HypotheticalCandidate>>
            known_hypotheticals);
    void handleFetchedStatementResponse(
        outcome::result<network::vstaging::AttestedCandidateResponse> &&r,
        const RelayHash &relay_parent,
        const CandidateHash &candidate_hash,
        Groups &&groups,
        GroupIndex group_index);
    ManifestImportSuccessOpt handle_incoming_manifest_common(
        const libp2p::peer::PeerId &peer_id,
        const CandidateHash &candidate_hash,
        const RelayHash &relay_parent,
        const ManifestSummary &manifest_summary,
        ParachainId para_id);
    network::vstaging::StatementFilter local_knowledge_filter(
        size_t group_size,
        GroupIndex group_index,
        const CandidateHash &candidate_hash,
        const StatementStore &statement_store);
    std::deque<network::VersionedValidatorProtocolMessage>
    acknowledgement_and_statement_messages(
        StatementStore &statement_store,
        const std::vector<ValidatorIndex> &group,
        const network::vstaging::StatementFilter &local_knowledge,
        const CandidateHash &candidate_hash,
        const RelayHash &relay_parent);
    std::deque<network::VersionedValidatorProtocolMessage>
    post_acknowledgement_statement_messages(
        const RelayHash &relay_parent,
        const StatementStore &statement_store,
        const std::vector<ValidatorIndex> &group,
        const CandidateHash &candidate_hash);
    void send_to_validators_group(
        const RelayHash &relay_parent,
        const std::deque<network::VersionedValidatorProtocolMessage> &messages);
    void circulate_statement(
        const RelayHash &relay_parent,
        const IndexedAndSigned<network::vstaging::CompactStatement> &statement);

    outcome::result<std::pair<CollatorId, ParachainId>> insertAdvertisement(
        network::PeerState &peer_data,
        const RelayHash &relay_parent,
        const ProspectiveParachainsModeOpt &relay_parent_mode,
        const std::optional<std::reference_wrapper<const CandidateHash>>
            &candidate_hash);

    bool isRelayParentInImplicitView(
        const RelayHash &relay_parent,
        const ProspectiveParachainsModeOpt &relay_parent_mode,
        const ImplicitView &implicit_view,
        const std::unordered_map<Hash, ProspectiveParachainsModeOpt>
            &active_leaves,
        ParachainId para_id);

    template <typename F>
    void requestPoV(const libp2p::peer::PeerInfo &peer_info,
                    const CandidateHash &candidate_hash,
                    F &&callback);

    std::optional<AttestedCandidate> attested_candidate(
        const RelayHash &relay_parent,
        const CandidateHash &digest,
        const TableContext &context,
        uint32_t minimum_backing_votes);

    std::optional<AttestedCandidate> attested(
        const network::CommittedCandidateReceipt &candidate,
        const BackingStore::StatementInfo &data,
        size_t validity_threshold);
    std::optional<BackingStore::BackedCandidate> table_attested_to_backed(
        AttestedCandidate &&attested, TableContext &table_context);
    outcome::result<std::optional<ValidatorSigner>> isParachainValidator(
        const primitives::BlockHash &relay_parent) const;

    /*
     * Logic.
     */
    std::optional<runtime::PersistedValidationData>
    requestProspectiveValidationData(const RelayHash &relay_parent,
                                     const Hash &parent_head_data_hash,
                                     ParachainId para_id);
    std::optional<runtime::PersistedValidationData>
    requestPersistedValidationData(const RelayHash &relay_parent,
                                   ParachainId para_id);

    std::optional<runtime::PersistedValidationData>
    fetchPersistedValidationData(const RelayHash &relay_parent,
                                 ParachainId para_id);
    void onValidationComplete(const libp2p::peer::PeerId &peer_id,
                              const ValidateAndSecondResult &result);
    void onAttestComplete(const libp2p::peer::PeerId &peer_id,
                          const ValidateAndSecondResult &result);
    void onAttestNoPoVComplete(const network::RelayHash &relay_parent,
                               const CandidateHash &candidate_hash);

    void kickOffValidationWork(
        const RelayHash &relay_parent,
        AttestingData &attesting_data,
        const runtime::PersistedValidationData &persisted_validation_data,
        RelayParentState &parachain_state);
    std::optional<runtime::SessionInfo> retrieveSessionInfo(
        const RelayHash &relay_parent);
    void handleFetchedCollation(PendingCollation &&pending_collation,
                                network::CollationFetchingResponse &&response);
    template <StatementType kStatementType>
    std::optional<network::SignedStatement> createAndSignStatement(
        const ValidateAndSecondResult &validation_result);
    template <ParachainProcessorImpl::StatementType kStatementType>
    outcome::result<
        std::optional<ParachainProcessorImpl::SignedFullStatementWithPVD>>
    sign_import_and_distribute_statement(
        ParachainProcessorImpl::RelayParentState &rp_state,
        const ValidateAndSecondResult &validation_result);
    void post_import_statement_actions(
        const RelayHash &relay_parent,
        ParachainProcessorImpl::RelayParentState &rp_state,
        std::optional<BackingStore::ImportResult> &summary);
    template <typename T>
    std::optional<network::SignedStatement> createAndSignStatementFromPayload(
        T &&payload,
        ValidatorIndex validator_ix,
        RelayParentState &parachain_state);
    outcome::result<std::optional<BackingStore::ImportResult>> importStatement(
        const network::RelayHash &relay_parent,
        const SignedFullStatementWithPVD &statement,
        ParachainProcessorImpl::RelayParentState &relayParentState);

    const network::CandidateDescriptor &candidateDescriptorFrom(
        const network::CollationFetchingResponse &collation) {
      return visit_in_place(
          collation.response_data,
          [](const network::CollationResponse &collation_response)
              -> const network::CandidateDescriptor & {
            return collation_response.receipt.descriptor;
          });
    }

    std::optional<std::reference_wrapper<const network::CandidateDescriptor>>
    candidateDescriptorFrom(const network::Statement &statement) {
      return visit_in_place(
          statement.candidate_state,
          [](const network::CommittedCandidateReceipt &receipt)
              -> std::optional<
                  std::reference_wrapper<const network::CandidateDescriptor>> {
            return receipt.descriptor;
          },
          [](...)
              -> std::optional<
                  std::reference_wrapper<const network::CandidateDescriptor>> {
            BOOST_ASSERT(false);
            return std::nullopt;
          });
    }

    const network::CollatorPublicKey &collatorIdFromDescriptor(
        const network::CandidateDescriptor &descriptor) {
      return descriptor.collator_id;
    }

    network::CandidateReceipt candidateFromCommittedCandidateReceipt(
        const network::CommittedCandidateReceipt &data) const {
      network::CandidateReceipt receipt;
      receipt.descriptor = data.descriptor,
      receipt.commitments_hash =
          hasher_->blake2b_256(scale::encode(data.commitments).value());
      return receipt;
    }

    primitives::BlockHash candidateHashFrom(
        const StatementWithPVD &statement) const {
      return visit_in_place(
          statement,
          [&](const StatementWithPVDSeconded &val) {
            return hasher_->blake2b_256(
                ::scale::encode(candidateFromCommittedCandidateReceipt(
                                    val.committed_receipt))
                    .value());
          },
          [&](const StatementWithPVDValid &val) { return val.candidate_hash; });
    }

    /*
     * Notification
     */
    void broadcastView(const network::View &view) const;
    void broadcastViewToGroup(const primitives::BlockHash &relay_parent,
                              const network::View &view);
    void broadcastViewExcept(const libp2p::peer::PeerId &peer_id,
                             const network::View &view) const;
    template <typename F>
    void notify_internal(std::shared_ptr<WorkersContext> &context, F &&func) {
      BOOST_ASSERT(context);
      boost::asio::post(*context, std::forward<F>(func));
    }
    void statementDistributionBackedCandidate(
        const CandidateHash &candidate_hash);
    void notifyAvailableData(std::vector<network::ErasureChunk> &&chunk_list,
                             const primitives::BlockHash &relay_parent,
                             const network::CandidateHash &candidate_hash,
                             const network::ParachainBlock &pov,
                             const runtime::PersistedValidationData &data);
    void share_local_statement_vstaging(
        RelayParentState &per_relay_parent,
        const primitives::BlockHash &relay_parent,
        const SignedFullStatementWithPVD &statement);
    void share_local_statement_v1(RelayParentState &per_relay_parent,
                                  const primitives::BlockHash &relay_parent,
                                  const SignedFullStatementWithPVD &statement);
    void notify(const libp2p::peer::PeerId &peer_id,
                const primitives::BlockHash &relay_parent,
                const SignedFullStatementWithPVD &statement);
    void handleNotify(const libp2p::peer::PeerId &peer_id,
                      const primitives::BlockHash &relay_parent);

    void onDeactivateBlocks(const primitives::events::ChainEventParams &event);
    void onViewUpdated(const network::ExView &event);
    void OnBroadcastBitfields(const primitives::BlockHash &relay_parent,
                              const network::SignedBitfield &bitfield);
    void onUpdatePeerView(const libp2p::peer::PeerId &peer_id,
                          const network::View &view);
    void fetchCollation(
        ParachainProcessorImpl::RelayParentState &per_relay_parent,
        PendingCollation &&pc,
        const CollatorId &id);
    void fetchCollation(
        ParachainProcessorImpl::RelayParentState &per_relay_parent,
        PendingCollation &&pc,
        const CollatorId &id,
        network::CollationVersion version);
    std::optional<std::reference_wrapper<RelayParentState>>
    tryGetStateByRelayParent(const primitives::BlockHash &relay_parent);
    RelayParentState &storeStateByRelayParent(
        const primitives::BlockHash &relay_parent, RelayParentState &&val);
    void send_peer_messages_for_relay_parent(
        std::optional<std::reference_wrapper<const libp2p::peer::PeerId>>
            peer_id,
        const RelayHash &relay_parent);

    void createBackingTask(const primitives::BlockHash &relay_parent);
    outcome::result<RelayParentState> initNewBackingTask(
        const primitives::BlockHash &relay_parent);

    template <typename F>
    bool tryOpenOutgoingCollatingStream(const libp2p::peer::PeerId &peer_id,
                                        F &&callback);
    template <typename F>
    bool tryOpenOutgoingValidationStream(const libp2p::peer::PeerId &peer_id,
                                         network::CollationVersion version,
                                         F &&callback);
    template <typename F>
    bool tryOpenOutgoingStream(const libp2p::peer::PeerId &peer_id,
                               std::shared_ptr<network::ProtocolBase> protocol,
                               F &&callback);

    outcome::result<void> enqueueCollation(
        ParachainProcessorImpl::RelayParentState &per_relay_parent,
        const RelayHash &relay_parent,
        ParachainId para_id,
        const libp2p::peer::PeerId &peer_id,
        const CollatorId &collator_id,
        std::optional<std::pair<CandidateHash, Hash>> &&prospective_candidate);
    void sendMyView(const libp2p::peer::PeerId &peer_id,
                    const std::shared_ptr<network::Stream> &stream,
                    const std::shared_ptr<network::ProtocolBase> &protocol);

    bool isValidatingNode() const;
    void unblockAdvertisements(
        ParachainProcessorImpl::RelayParentState &rp_state,
        ParachainId para_id,
        const Hash &para_head);
    void requestUnblockedCollations(
        ParachainProcessorImpl::RelayParentState &rp_state,
        ParachainId para_id,
        const Hash &para_head,
        std::vector<BlockedAdvertisement> &&unblocked);

    bool canSecond(ParachainProcessorImpl::RelayParentState &per_relay_parent,
                   ParachainId candidate_para_id,
                   const Hash &candidate_relay_parent,
                   const CandidateHash &candidate_hash,
                   const Hash &parent_head_data_hash);

    ParachainProcessorImpl::SecondingAllowed secondingSanityCheck(
        const HypotheticalCandidate &hypothetical_candidate,
        bool backed_in_path_only);

    std::optional<BackingStore::ImportResult> importStatementToTable(
        const RelayHash &relay_parent,
        ParachainProcessorImpl::RelayParentState &relayParentState,
        const primitives::BlockHash &candidate_hash,
        const network::SignedStatement &statement);

    std::shared_ptr<network::PeerManager> pm_;
    std::shared_ptr<dispute::RuntimeInfo> runtime_info_;
    std::shared_ptr<crypto::Sr25519Provider> crypto_provider_;
    std::shared_ptr<network::Router> router_;
    log::Logger logger_ =
        log::createLogger("ParachainProcessorImpl", "parachain");

    struct {
      std::unordered_map<primitives::BlockHash, RelayParentState>
          state_by_relay_parent;
      std::unordered_map<
          libp2p::peer::PeerId,
          std::deque<std::pair<RelayHash, SignedFullStatementWithPVD>>>
          seconded_statements;
      std::optional<ImplicitView> implicit_view;
      std::unordered_map<Hash, ActiveLeafState> per_leaf;
      std::unordered_map<CandidateHash, PerCandidateState> per_candidate;
      /// Added as independent member to prevent extra locks for
      /// `state_by_relay_parent` which is used in internal thread only
      std::unordered_map<Hash, ProspectiveParachainsModeOpt> active_leaves;
      std::unordered_map<
          ParachainId,
          std::unordered_map<Hash, std::vector<BlockedAdvertisement>>>
          blocked_advertisements;
      std::unordered_set<PendingCollation,
                         PendingCollationHash,
                         PendingCollationEq>
          collation_requests_cancel_handles;

      struct {
        std::unordered_set<Hash> implicit_view;
        network::View view;
      } statementDistributionV2;
    } our_current_state_;

    std::unordered_map<RelayHash, PendingCollation> pending_candidates;
    WeakIoContext main_thread_context_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<network::PeerView> peer_view_;
    network::PeerView::MyViewSubscriberPtr my_view_sub_;
    network::PeerView::PeerViewSubscriberPtr remote_view_sub_;

    std::shared_ptr<parachain::Pvf> pvf_;
    std::shared_ptr<parachain::ValidatorSignerFactory> signer_factory_;
    std::shared_ptr<parachain::BitfieldSigner> bitfield_signer_;
    std::shared_ptr<parachain::PvfPrecheck> pvf_precheck_;
    std::shared_ptr<parachain::BitfieldStore> bitfield_store_;
    std::shared_ptr<parachain::BackingStore> backing_store_;
    std::shared_ptr<parachain::AvailabilityStore> av_store_;
    std::shared_ptr<runtime::ParachainHost> parachain_host_;
    const application::AppConfiguration &app_config_;
    primitives::events::BabeStateSubscriptionEnginePtr babe_status_observable_;
    primitives::events::BabeStateEventSubscriberPtr babe_status_observer_;
    std::shared_ptr<authority_discovery::Query> query_audi_;

    std::shared_ptr<primitives::events::ChainEventSubscriber> chain_sub_;
    WeakIoContext worker_thread_context_;
    std::default_random_engine random_;
    std::shared_ptr<ProspectiveParachains> prospective_parachains_;
    Candidates candidates_;

    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_is_parachain_validator_;
  };

}  // namespace kagome::parachain

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain, ParachainProcessorImpl::Error);
