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
#include "common/ref_cache.hpp"
#include "common/visitor.hpp"
#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/timeline/slots_util.hpp"
#include "crypto/hasher.hpp"
#include "metrics/metrics.hpp"
#include "network/can_disconnect.hpp"
#include "network/peer_manager.hpp"
#include "network/peer_view.hpp"
#include "network/protocols/req_collation_protocol.hpp"
#include "network/types/collator_messages_vstaging.hpp"
#include "outcome/outcome.hpp"
#include "parachain/availability/bitfield/signer.hpp"
#include "parachain/availability/store/store.hpp"
#include "parachain/backing/cluster.hpp"
#include "parachain/backing/grid_tracker.hpp"
#include "parachain/backing/store.hpp"
#include "parachain/pvf/precheck.hpp"
#include "parachain/pvf/pvf.hpp"
#include "parachain/validator/backing_implicit_view.hpp"
#include "parachain/validator/collations.hpp"
#include "parachain/validator/impl/candidates.hpp"
#include "parachain/validator/impl/statements_store.hpp"
#include "parachain/validator/prospective_parachains/prospective_parachains.hpp"
#include "parachain/validator/signer.hpp"
#include "primitives/common.hpp"
#include "primitives/event_types.hpp"
#include "utils/non_copyable.hpp"
#include "utils/safe_object.hpp"

/**
 * @file parachain_processor_impl.hpp
 * The ParachainProcessorImpl class is responsible for handling the validation
 * and processing of parachains in the network. It manages the lifecycle of
 * parachains, including their creation, validation, and destruction.
 *
 * The class contains methods for handling incoming streams of data, processing
 * parachain blocks, and managing peer states. It also handles the validation of
 * candidates for the parachain, including checking the validity of the erasure
 * coding and the availability of data.
 *
 * In addition, the class manages the communication with
 * other nodes in the network, sending and receiving messages related to the
 * state of the parachains. It also handles the storage and retrieval of data
 * related to the parachains.
 *
 * The class uses a variety of helper methods and
 * data structures to perform its tasks, including a main pool handler for
 * managing tasks, a logger for logging events, and various data structures for
 * storing the state of the parachains and the peers in the network
 */
namespace kagome {
  class ThreadHandler;
}

namespace kagome::common {
  class MainThreadPool;
  class WorkerThreadPool;
}  // namespace kagome::common

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
        network::CanDisconnect,
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
      REJECTED_BY_PROSPECTIVE_PARACHAINS,
      INCORRECT_BITFIELD_SIZE,
      CORE_INDEX_UNAVAILABLE,
      INCORRECT_SIGNATURE,
      CLUSTER_TRACKER_ERROR,
      PERSISTED_VALIDATION_DATA_NOT_FOUND,
      PERSISTED_VALIDATION_DATA_MISMATCH,
      CANDIDATE_HASH_MISMATCH,
      PARENT_HEAD_DATA_MISMATCH,
      NO_PEER,
      ALREADY_REQUESTED,
      NOT_ADVERTISED,
      WRONG_PARA,
      THRESHOLD_LIMIT_REACHED,
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
        common::MainThreadPool &main_thread_pool,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<network::PeerView> peer_view,
        common::WorkerThreadPool &worker_thread_pool,
        std::shared_ptr<parachain::BitfieldSigner> bitfield_signer,
        std::shared_ptr<parachain::PvfPrecheck> pvf_precheck,
        std::shared_ptr<parachain::BitfieldStore> bitfield_store,
        std::shared_ptr<parachain::BackingStore> backing_store,
        std::shared_ptr<parachain::Pvf> pvf,
        std::shared_ptr<parachain::AvailabilityStore> av_store,
        std::shared_ptr<runtime::ParachainHost> parachain_host,
        std::shared_ptr<parachain::ValidatorSignerFactory> signer_factory,
        const application::AppConfiguration &app_config,
        application::AppStateManager &app_state_manager,
        primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
        primitives::events::SyncStateSubscriptionEnginePtr
            sync_state_observable,
        std::shared_ptr<authority_discovery::Query> query_audi,
        std::shared_ptr<ProspectiveParachains> prospective_parachains,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        LazySPtr<consensus::SlotsUtil> slots_util,
        std::shared_ptr<consensus::babe::BabeConfigRepository>
            babe_config_repo);
    ~ParachainProcessorImpl() = default;

    /**
     * @brief Prepares the Parachain Processor for operation.
     *
     * This method is responsible for setting up necessary configurations and
     * initializations for the Parachain Processor. It should be called before
     * the Parachain Processor starts processing parachain blocks.
     *
     * @return Returns true if the preparation is successful, false otherwise.
     */
    bool prepare();

    /**
     * @brief Handles an incoming advertisement for a collation.
     *
     * @param pending_collation The CollationEvent representing the collation
     * being advertised.
     * @param prospective_candidate An optional pair containing the hash of the
     * prospective candidate and the hash of the parent block.
     */
    void handleAdvertisement(
        const RelayHash &relay_parent,
        const libp2p::peer::PeerId &peer_id,
        std::optional<std::pair<CandidateHash, Hash>> &&prospective_candidate);

    /**
     * @ brief We should only process parachains if we are validator and we are
     * @return outcome::result<void> Returns an error if we cannot process the
     * parachains.
     */
    outcome::result<void> canProcessParachains() const;

    /**
     * @brief Handles an incoming collator.
     *
     * This function is called when a new collator is detected. It updates the
     * internal state with the information about the new collator.
     *
     * @param peer_id The peer id of the collator.
     * @param pubkey The public key of the collator.
     * @param para_id The id of the parachain the collator is associated with.
     */
    void onIncomingCollator(const libp2p::peer::PeerId &peer_id,
                            network::CollatorPublicKey pubkey,
                            network::ParachainId para_id);

    /**
     * @brief Handles an incoming collation stream from a peer.
     *
     * @param peer_id The ID of the peer from which the collation stream is
     * received.
     * @param version The version of the collation protocol used in the stream.
     */
    void onIncomingCollationStream(const libp2p::peer::PeerId &peer_id,
                                   network::CollationVersion version);

    /**
     * @brief Handles an incoming validation stream from a peer.
     *
     * @param peer_id The ID of the peer from which the validation stream is
     * received.
     * @param version The version of the collation protocol used in the
     * validation stream.
     */
    void onIncomingValidationStream(const libp2p::peer::PeerId &peer_id,
                                    network::CollationVersion version);

    void onValidationProtocolMsg(
        const libp2p::peer::PeerId &peer_id,
        const network::VersionedValidatorProtocolMessage &message);

    outcome::result<network::FetchChunkResponse> OnFetchChunkRequest(
        const network::FetchChunkRequest &request);

    outcome::result<network::vstaging::AttestedCandidateResponse>
    OnFetchAttestedCandidateRequest(
        const network::vstaging::AttestedCandidateRequest &request,
        const libp2p::peer::PeerId &peer_id);
    outcome::result<BlockNumber> get_block_number_under_construction(
        const RelayHash &relay_parent) const;
    bool bitfields_indicate_availability(
        size_t core_idx,
        const std::vector<BitfieldStore::SignedBitfield> &bitfields,
        const scale::BitVec &availability);

    /**
     * @brief Fetches the list of backed candidates for a given relay parent.
     *
     * @param relay_parent The hash of the relay chain block where the parachain
     * block is attached.
     * @return std::vector<network::BackedCandidate> A vector of backed
     * candidates for the given relay parent.
     */
    std::vector<network::BackedCandidate> getBackedCandidates(
        const RelayHash &relay_parent) override;

    // CanDisconnect
    bool canDisconnect(const libp2p::PeerId &) const override;

    /**
     * @brief Fetches the Proof of Validity (PoV) for a given candidate.
     *
     * @param candidate_hash The hash of the candidate for which the PoV is to
     * be fetched.
     * @return network::ResponsePov The PoV associated with the given candidate
     * hash.
     */
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
    using SecondingAllowed = std::optional<std::vector<Hash>>;

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
      std::unordered_map<CoreIndex, std::vector<ValidatorIndex>> groups;
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

    /**
     * @brief Converts a SignedFullStatementWithPVD to an IndexedAndSigned
     * CompactStatement.
     */
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

    /// polkadot/node/network/statement-distribution/src/v2/mod.rs
    struct ActiveValidatorState {
      // The index of the validator.
      ValidatorIndex index;
      // our validator group
      GroupIndex group;
      // the assignment of our validator group, if any.
      std::optional<ParachainId> assignment;
      // the 'direct-in-group' communication at this relay-parent.
      ClusterTracker cluster_tracker;
    };

    struct LocalValidatorState {
      grid::GridTracker grid_tracker;
      std::optional<ActiveValidatorState> active;
    };

    using PeerUseCount = SafeObject<
        std::unordered_map<primitives::AuthorityDiscoveryId, size_t>>;

    struct PerSessionState {
      PerSessionState(const PerSessionState &) = delete;
      PerSessionState &operator=(const PerSessionState &) = delete;

      PerSessionState(PerSessionState &&) = default;
      PerSessionState &operator=(PerSessionState &&) = delete;

      SessionIndex session;
      runtime::SessionInfo session_info;
      Groups groups;
      std::optional<grid::Views> grid_view;
      std::optional<ValidatorIndex> our_index;
      std::optional<GroupIndex> our_group;
      std::shared_ptr<PeerUseCount> peers;

      PerSessionState(SessionIndex _session,
                      const runtime::SessionInfo &_session_info,
                      Groups &&_groups,
                      grid::Views &&_grid_view,
                      std::optional<ValidatorIndex> _our_index,
                      std::shared_ptr<PeerUseCount> peers);
      ~PerSessionState();
      void updatePeers(bool add) const;
    };

    struct RelayParentState {
      ProspectiveParachainsModeOpt prospective_parachains_mode;
      std::optional<CoreIndex> assigned_core;
      std::optional<ParachainId> assigned_para;
      std::vector<std::optional<GroupIndex>> validator_to_group;
      std::shared_ptr<RefCache<SessionIndex, PerSessionState>::RefObj>
          per_session_state;

      std::optional<network::ValidatorIndex> our_index;
      std::optional<network::GroupIndex> our_group;

      Collations collations;
      TableContext table_context;
      std::optional<StatementStore> statement_store;
      std::vector<runtime::CoreState> availability_cores;
      runtime::GroupDescriptor group_rotation_info;
      uint32_t minimum_backing_votes;
      std::unordered_map<primitives::AuthorityDiscoveryId, ValidatorIndex>
          authority_lookup;
      std::optional<LocalValidatorState> local_validator;

      std::unordered_set<primitives::BlockHash> awaiting_validation;
      std::unordered_set<primitives::BlockHash> issued_statements;
      std::unordered_set<network::PeerId> peers_advertised;
      std::unordered_map<primitives::BlockHash, AttestingData> fallbacks;
      std::unordered_set<CandidateHash> backed_hashes;
      std::unordered_set<ValidatorIndex> disabled_validators;

      bool inject_core_index;

      bool is_disabled(ValidatorIndex validator_index) const {
        return disabled_validators.contains(validator_index);
      }

      scale::BitVec disabled_bitmask(
          const std::span<const ValidatorIndex> &group) const {
        scale::BitVec v;
        v.bits.resize(group.size());
        for (size_t ix = 0; ix < group.size(); ++ix) {
          v.bits[ix] = is_disabled(group[ix]);
        }
        return v;
      }
    };

    /**
     * @struct PerCandidateState
     * @brief This structure represents the state of a candidate in the
     * parachain validation process.
     */
    struct PerCandidateState {
      runtime::PersistedValidationData persisted_validation_data;
      bool seconded_locally;
      ParachainId para_id;
      Hash relay_parent;
    };

    using ManifestSummary = parachain::grid::ManifestSummary;

    struct ManifestImportSuccess {
      bool acknowledge;
      ValidatorIndex sender_index;
    };
    using ManifestImportSuccessOpt = std::optional<ManifestImportSuccess>;

    /*
     * Validation.
     */

    /**
     * @brief Validates the erasure coding of the provided data.
     *
     * This function takes the available data from a runtime and the number of
     * validators, and validates the erasure coding of the data.
     *
     * @param validating_data The available data from a runtime that needs to be
     * validated.
     * @param n_validators The number of validators that will be used for the
     * validation process.
     * @return Returns a vector of erasure chunks if the validation is
     * successful, otherwise returns an error.
     */
    outcome::result<std::vector<network::ErasureChunk>> validateErasureCoding(
        const runtime::AvailableData &validating_data, size_t n_validators);

    /**
     * @brief This function is a template function that validates a candidate
     * asynchronously.
     *
     * @tparam kMode The type of validation task to be performed.
     *
     * @param candidate The candidate receipt to be validated.
     * @param pov The parachain block to be validated.
     * @param pvd The persisted validation data to be used in the validation
     * process.
     * @param peer_id The peer ID of the node performing the validation.
     * @param relay_parent The block hash of the relay parent.
     * @param n_validators The number of validators in the network.
     */
    template <ParachainProcessorImpl::ValidationTaskType kMode>
    void validateAsync(network::CandidateReceipt &&candidate,
                       network::ParachainBlock &&pov,
                       runtime::PersistedValidationData &&pvd,
                       const primitives::BlockHash &relay_parent);

    /**
     * @brief This function is used to make a candidate available for
     * validation.
     *
     * @tparam kMode The type of validation task to be performed. It can be
     * either 'Second' or 'Attest'.
     * @param peer_id The ID of the peer that the candidate is being made
     * available to.
     * @param candidate_hash The hash of the candidate that is being made
     * available.
     * @param result The result of the validation and seconding process.
     */
    template <ParachainProcessorImpl::ValidationTaskType kMode>
    void makeAvailable(const primitives::BlockHash &candidate_hash,
                       ValidateAndSecondResult &&result);

    /**
     * @brief Handles a statement related to a specific relay parent.
     *
     * This function is responsible for processing a signed statement associated
     * with a specific relay parent. The statement is provided in the form of a
     * SignedFullStatementWithPVD object, which includes the payload and
     * signature. The relay parent is identified by its block hash.
     *
     * @param relay_parent The block hash of the relay parent associated with
     * the statement.
     * @param statement The signed statement to be processed, encapsulated in a
     * SignedFullStatementWithPVD object.
     */
    void handleStatement(const primitives::BlockHash &relay_parent,
                         const SignedFullStatementWithPVD &statement);

    /**
     * @brief Processes a bitfield distribution message.
     *
     * This function is responsible for handling a bitfield distribution message
     * received from the network. The bitfield distribution message contains
     * information about the availability of pieces of data in the network.
     */
    void process_bitfield_distribution(
        const network::BitfieldDistributionMessage &val);

    void process_legacy_statement(
        const libp2p::peer::PeerId &peer_id,
        const network::StatementDistributionMessage &msg);
    outcome::result<void> handle_grid_statement(
        const RelayHash &relay_parent,
        ParachainProcessorImpl::RelayParentState &per_relay_parent,
        grid::GridTracker &grid_tracker,
        const IndexedAndSigned<network::vstaging::CompactStatement> &statement,
        ValidatorIndex grid_sender_index);

    /**
     * @brief Processes a vstaging statement.
     *
     * This function is responsible for processing a vstaging statement received
     * from a peer. It handles different types of messages:
     * BackedCandidateAcknowledgement, BackedCandidateManifest, and
     * StatementDistributionMessageStatement. Depending on the type of the
     * message, it performs different actions such as handling incoming
     * manifest, sending acknowledgement and statement messages, fetching
     * attested candidate response, inserting statement to the store, and
     * circulating the statement.
     *
     * @param peer_id The id of the peer from which the statement is received.
     * @param msg The vstaging statement message to be processed.
     */
    void process_vstaging_statement(
        const libp2p::peer::PeerId &peer_id,
        const network::vstaging::StatementDistributionMessage &msg);

    /// Checks whether a statement is allowed, whether the signature is
    /// accurate,
    /// and importing into the cluster tracker if successful.
    ///
    /// if successful, this returns a checked signed statement if it should be
    /// imported or otherwise an error indicating a reputational fault.
    outcome::result<std::optional<network::vstaging::SignedCompactStatement>>
    handle_cluster_statement(
        const RelayHash &relay_parent,
        ClusterTracker &cluster_tracker,
        SessionIndex session,
        const runtime::SessionInfo &session_info,
        const network::vstaging::SignedCompactStatement &statement,
        ValidatorIndex cluster_sender_index);

    /// Check a statement signature under this parent hash.
    outcome::result<
        std::reference_wrapper<const network::vstaging::SignedCompactStatement>>
    check_statement_signature(
        SessionIndex session_index,
        const std::vector<ValidatorId> &validators,
        const RelayHash &relay_parent,
        const network::vstaging::SignedCompactStatement &statement);
    void handle_incoming_manifest(
        const libp2p::peer::PeerId &peer_id,
        const network::vstaging::BackedCandidateManifest &msg);
    void handle_incoming_statement(
        const libp2p::peer::PeerId &peer_id,
        const network::vstaging::StatementDistributionMessageStatement &stm);
    void handle_incoming_acknowledgement(
        const libp2p::peer::PeerId &peer_id,
        const network::vstaging::BackedCandidateAcknowledgement
            &acknowledgement);
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
    void send_cluster_candidate_statements(const CandidateHash &candidate_hash,
                                           const RelayHash &relay_parent);
    void new_confirmed_candidate_fragment_chain_updates(
        const HypotheticalCandidate &candidate);
    void new_leaf_fragment_chain_updates(const Hash &leaf_hash);
    void prospective_backed_notification_fragment_chain_updates(
        ParachainId para_id, const Hash &para_head);
    void fragment_chain_update_inner(
        std::optional<std::reference_wrapper<const Hash>> active_leaf_hash,
        std::optional<std::pair<std::reference_wrapper<const Hash>,
                                ParachainId>> required_parent_info,
        std::optional<std::reference_wrapper<const HypotheticalCandidate>>
            known_hypotheticals);
    void handleFetchedStatementResponse(
        outcome::result<network::vstaging::AttestedCandidateResponse> &&r,
        const RelayHash &relay_parent,
        const CandidateHash &candidate_hash,
        GroupIndex group_index);
    outcome::result<consensus::Randomness> getBabeRandomness(
        const RelayHash &relay_parent);
    outcome::result<std::optional<runtime::ClaimQueueSnapshot>>
    fetch_claim_queue(const RelayHash &relay_parent);
    void request_attested_candidate(const libp2p::peer::PeerId &peer,
                                    RelayParentState &relay_parent_state,
                                    const RelayHash &relay_parent,
                                    const CandidateHash &candidate_hash,
                                    GroupIndex group_index);
    ManifestImportSuccessOpt handle_incoming_manifest_common(
        const libp2p::peer::PeerId &peer_id,
        const CandidateHash &candidate_hash,
        const RelayHash &relay_parent,
        ManifestSummary manifest_summary,
        ParachainId para_id,
        grid::ManifestKind manifest_kind);
    network::vstaging::StatementFilter local_knowledge_filter(
        size_t group_size,
        GroupIndex group_index,
        const CandidateHash &candidate_hash,
        const StatementStore &statement_store);
    std::deque<std::pair<std::vector<libp2p::peer::PeerId>,
                         network::VersionedValidatorProtocolMessage>>
    acknowledgement_and_statement_messages(
        const libp2p::peer::PeerId &peer,
        network::CollationVersion version,
        ValidatorIndex validator_index,
        const Groups &groups,
        ParachainProcessorImpl::RelayParentState &relay_parent_state,
        const RelayHash &relay_parent,
        GroupIndex group_index,
        const CandidateHash &candidate_hash,
        const network::vstaging::StatementFilter &local_knowledge);
    std::deque<network::VersionedValidatorProtocolMessage>
    post_acknowledgement_statement_messages(
        ValidatorIndex recipient,
        const RelayHash &relay_parent,
        grid::GridTracker &grid_tracker,
        const StatementStore &statement_store,
        const Groups &groups,
        GroupIndex group_index,
        const CandidateHash &candidate_hash,
        const libp2p::peer::PeerId &peer,
        network::CollationVersion version);
    void send_to_validators_group(
        const RelayHash &relay_parent,
        const std::deque<network::VersionedValidatorProtocolMessage> &messages);

    /**
     * @brief Circulates a statement to the validators group.
     * @param relay_parent The hash of the relay parent block. This is used to
     * identify the group of validators to which the statement should be sent.
     * @param statement The statement to be circulated. This is an indexed and
     * signed compact statement.
     */
    void circulate_statement(
        const RelayHash &relay_parent,
        RelayParentState &relay_parent_state,
        const IndexedAndSigned<network::vstaging::CompactStatement> &statement);

    /**
     * @brief Inserts an advertisement into the peer's data.
     *
     * This function is responsible for inserting an advertisement into the
     * peer's data. It performs several checks to ensure that the advertisement
     * can be inserted, such as checking if the collator is declared, if the
     * relay parent is in the implicit view, and if there are any duplicates. If
     * all checks pass, the advertisement is inserted and the function returns
     * the collator ID and parachain ID.
     *
     * @param peer_data The peer's data where the advertisement will be
     * inserted.
     * @param on_relay_parent The hash of the relay parent block.
     * @param relay_parent_mode The mode of the relay parent.
     * @param candidate_hash The hash of the candidate block.
     * @return A pair containing the collator ID and the parachain ID if the
     * advertisement was inserted successfully, or an error otherwise.
     */
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
    void requestPoV(const libp2p::peer::PeerId &peer_id,
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
        AttestedCandidate &&attested,
        TableContext &table_context,
        bool inject_core_index);
    outcome::result<std::optional<ValidatorSigner>> isParachainValidator(
        const primitives::BlockHash &relay_parent) const;

    /*
     * Logic.
     */
    /// Dequeue another collation and fetch.
    void dequeue_next_collation_and_fetch(
        const RelayHash &relay_parent,
        std::pair<CollatorId, std::optional<CandidateHash>> previous_fetch);

    outcome::result<std::optional<runtime::PersistedValidationData>>
    requestProspectiveValidationData(
        const RelayHash &relay_parent,
        const Hash &parent_head_data_hash,
        ParachainId para_id,
        const std::optional<HeadData> &maybe_parent_head_data);
    outcome::result<std::optional<runtime::PersistedValidationData>>
    requestPersistedValidationData(const RelayHash &relay_parent,
                                   ParachainId para_id);

    /// Performs a sanity check between advertised and fetched collations.
    /// Since the persisted validation data is constructed using the advertised
    /// parent head data hash, the latter doesn't require an additional check.
    outcome::result<void> fetched_collation_sanity_check(
        const PendingCollation &advertised,
        const network::CandidateReceipt &fetched,
        const crypto::Hashed<const runtime::PersistedValidationData &,
                             32,
                             crypto::Blake2b_StreamHasher<32>>
            &persisted_validation_data,
        std::optional<std::pair<HeadData, Hash>> maybe_parent_head_and_hash);

    outcome::result<std::optional<runtime::PersistedValidationData>>
    fetchPersistedValidationData(const RelayHash &relay_parent,
                                 ParachainId para_id);
    void onValidationComplete(const ValidateAndSecondResult &result);
    void onAttestComplete(const ValidateAndSecondResult &result);
    void onAttestNoPoVComplete(const network::RelayHash &relay_parent,
                               const CandidateHash &candidate_hash);

    void kickOffValidationWork(
        const RelayHash &relay_parent,
        AttestingData &attesting_data,
        const runtime::PersistedValidationData &persisted_validation_data,
        RelayParentState &parachain_state);
    void handle_collation_fetch_response(
        network::CollationEvent &&collation_event,
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
    std::optional<CoreIndex> core_index_from_statement(
        const std::vector<std::optional<GroupIndex>> &validator_to_group,
        const runtime::GroupDescriptor &group_rotation_info,
        const std::vector<runtime::CoreState> &cores,
        const SignedFullStatementWithPVD &statement);
    const network::CandidateDescriptor &candidateDescriptorFrom(
        const network::CollationFetchingResponse &collation) {
      return visit_in_place(collation.response_data,
                            [](const auto &collation_response)
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

    primitives::BlockHash candidateHashFrom(
        const StatementWithPVD &statement) const {
      return visit_in_place(
          statement,
          [&](const StatementWithPVDSeconded &val) {
            return hasher_->blake2b_256(
                ::scale::encode(val.committed_receipt.to_plain(*hasher_))
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
    template <bool kReinvoke = true>
    void notifySeconded(const primitives::BlockHash &relay_parent,
                        const SignedFullStatementWithPVD &statement);

    template <bool kReinvoke = true>
    void notifyInvalid(const primitives::BlockHash &parent,
                       const network::CandidateReceipt &candidate_receipt);

    /// Notify a collator that its collation got seconded.
    void notify_collation_seconded(const libp2p::peer::PeerId &peer_id,
                                   network::CollationVersion version,
                                   const RelayHash &relay_parent,
                                   const SignedFullStatementWithPVD &statement);

    void onDeactivateBlocks(
        const primitives::events::RemoveAfterFinalizationParams &event);
    void onViewUpdated(const network::ExView &event);
    void OnBroadcastBitfields(const primitives::BlockHash &relay_parent,
                              const network::SignedBitfield &bitfield);
    void onUpdatePeerView(const libp2p::peer::PeerId &peer_id,
                          const network::View &view);
    outcome::result<void> fetchCollation(const PendingCollation &pc,
                                         const CollatorId &id);
    outcome::result<void> fetchCollation(const PendingCollation &pc,
                                         const CollatorId &id,
                                         network::CollationVersion version);
    std::optional<std::reference_wrapper<RelayParentState>>
    tryGetStateByRelayParent(const primitives::BlockHash &relay_parent);
    outcome::result<
        std::reference_wrapper<ParachainProcessorImpl::RelayParentState>>
    getStateByRelayParent(const primitives::BlockHash &relay_parent);

    /**
     * @brief Store the state of the relay parent.
     * @param relay_parent The hash of the relay parent block for which the
     * state is to be stored.
     * @param val The state of the relay parent block to be stored.
     * @return A reference to the stored state of the relay parent block.
     */
    RelayParentState &storeStateByRelayParent(
        const primitives::BlockHash &relay_parent, RelayParentState &&val);

    /**
     * @brief Sends peer messages corresponding for a given relay parent.
     *
     * @param peer_id Optional reference to the PeerId of the peer to send the
     * messages to.
     * @param relay_parent The hash of the relay parent block
     */
    void send_peer_messages_for_relay_parent(
        const libp2p::peer::PeerId &peer_id, const RelayHash &relay_parent);
    std::optional<std::pair<std::vector<libp2p::peer::PeerId>,
                            network::VersionedValidatorProtocolMessage>>
    pending_statement_network_message(
        const StatementStore &statement_store,
        const RelayHash &relay_parent,
        const libp2p::peer::PeerId &peer,
        network::CollationVersion version,
        ValidatorIndex originator,
        const network::vstaging::CompactStatement &compact);
    void prune_old_advertisements(
        const parachain::ImplicitView &implicit_view,
        const std::unordered_map<Hash, ProspectiveParachainsModeOpt>
            &active_leaves,
        const std::unordered_map<primitives::BlockHash, RelayParentState>
            &per_relay_parent);
    void provide_candidate_to_grid(
        const CandidateHash &candidate_hash,
        RelayParentState &relay_parent_state,
        const ConfirmedCandidate &confirmed_candidate,
        const runtime::SessionInfo &session_info);
    void send_pending_cluster_statements(
        const RelayHash &relay_parent,
        const libp2p::peer::PeerId &peer_id,
        network::CollationVersion version,
        ValidatorIndex peer_validator_id,
        ParachainProcessorImpl::RelayParentState &relay_parent_state);
    void send_pending_grid_messages(
        const RelayHash &relay_parent,
        const libp2p::peer::PeerId &peer_id,
        network::CollationVersion version,
        ValidatorIndex peer_validator_id,
        const Groups &groups,
        ParachainProcessorImpl::RelayParentState &relay_parent_state);

    /**
     * The `create_backing_task` function is responsible for creating a new
     * backing task for a given relay parent. It first asserts that the function
     * is running in the main thread context. Then, it initializes a new backing
     * task for the relay parent by calling the
     * `construct_per_relay_parent_state` function. If the initialization is
     * successful, it stores the state of the relay parent by calling the
     * `storeStateByRelayParent` function. If the initialization fails and the
     * error is not due to the absence of a key, it logs an error message.
     *
     * @param relay_parent The hash of the relay parent block for which the
     * backing task is to be created.
     */
    void create_backing_task(
        const primitives::BlockHash &relay_parent,
        const network::HashedBlockHeader &block_header,
        const std::vector<primitives::BlockHash> &deactivated);

    /**
     * @brief The `construct_per_relay_parent_state` function is responsible for
     * initializing a new backing task for a given relay parent.
     * @param relay_parent The hash of the relay parent block for which the
     * backing task is to be created.
     * @return A `RelayParentState` object that contains the assignment,
     * validator index, required collator, and table context.
     */
    outcome::result<RelayParentState> construct_per_relay_parent_state(
        const primitives::BlockHash &relay_parent,
        const ProspectiveParachainsModeOpt &mode);

    void spawn_and_update_peer(const primitives::AuthorityDiscoveryId &id);

    std::optional<ParachainProcessorImpl::LocalValidatorState>
    find_active_validator_state(
        ValidatorIndex validator_index,
        const Groups &groups,
        const std::vector<runtime::CoreState> &availability_cores,
        const runtime::GroupDescriptor &group_rotation_info,
        const std::optional<runtime::ClaimQueueSnapshot> &maybe_claim_queue,
        size_t seconding_limit,
        size_t max_candidate_depth);

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
        std::unordered_map<
            ParachainId,
            std::unordered_map<Hash, std::vector<BlockedAdvertisement>>>
            &&unblocked);

    bool canSecond(ParachainId candidate_para_id,
                   const Hash &candidate_relay_parent,
                   const CandidateHash &candidate_hash,
                   const Hash &parent_head_data_hash);

    ParachainProcessorImpl::SecondingAllowed seconding_sanity_check(
        const HypotheticalCandidate &hypothetical_candidate);

    void printStoragesLoad();

    /// Handle a fetched collation result.
    /// polkadot/node/network/collator-protocol/src/validator_side/mod.rs
    outcome::result<void> kick_off_seconding(
        network::PendingCollationFetch &&pending_collation_fetch);

    std::optional<BackingStore::ImportResult> importStatementToTable(
        const RelayHash &relay_parent,
        ParachainProcessorImpl::RelayParentState &relayParentState,
        GroupIndex group_id,
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
        std::unordered_map<network::FetchedCollation, network::CollationEvent>
            fetched_candidates;
      } validator_side;
    } our_current_state_;

    std::shared_ptr<PoolHandler> main_pool_handler_;
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
    primitives::events::SyncStateSubscriptionEnginePtr sync_state_observable_;
    primitives::events::SyncStateEventSubscriberPtr sync_state_observer_;
    std::shared_ptr<authority_discovery::Query> query_audi_;
    std::shared_ptr<RefCache<SessionIndex, PerSessionState>> per_session_;
    std::shared_ptr<PeerUseCount> peer_use_count_;
    LazySPtr<consensus::SlotsUtil> slots_util_;
    std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo_;

    primitives::events::ChainSub chain_sub_;
    std::shared_ptr<PoolHandler> worker_pool_handler_;
    std::default_random_engine random_;
    std::shared_ptr<ProspectiveParachains> prospective_parachains_;
    Candidates candidates_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;

    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_is_parachain_validator_;
  };

}  // namespace kagome::parachain

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain, ParachainProcessorImpl::Error);
