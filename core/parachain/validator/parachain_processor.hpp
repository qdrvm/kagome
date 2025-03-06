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
#include "network/i_peer_view.hpp"
#include "network/peer_manager.hpp"
#include "network/protocols/req_collation_protocol.hpp"
#include "network/router.hpp"
#include "network/types/collator_messages_vstaging.hpp"
#include "outcome/outcome.hpp"
#include "parachain/availability/bitfield/signer.hpp"
#include "parachain/backing/cluster.hpp"
#include "parachain/backing/store.hpp"
#include "parachain/pvf/precheck.hpp"
#include "parachain/pvf/pvf.hpp"
#include "parachain/validator/backed_candidates_source.hpp"
#include "parachain/validator/backing_implicit_view.hpp"
#include "parachain/validator/collations.hpp"
#include "parachain/validator/i_parachain_processor.hpp"
#include "parachain/validator/parachain_storage.hpp"
#include "parachain/validator/prospective_parachains/prospective_parachains.hpp"
#include "parachain/validator/signer.hpp"
#include "parachain/validator/statement_distribution/i_statement_distribution.hpp"
#include "parachain/validator/statement_distribution/types.hpp"
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
  struct BlockedCollationId {
    /// Para id.
    ParachainId para_id;
    /// Hash of the parent head data.
    Hash parent_head_data_hash;

    BlockedCollationId(ParachainId pid, const Hash &h)
        : para_id(pid), parent_head_data_hash(h) {}
    auto operator<=>(const BlockedCollationId &) const = default;
    bool operator==(const BlockedCollationId &) const = default;
  };
}  // namespace kagome::parachain

template <>
struct std::hash<kagome::parachain::BlockedCollationId> {
  size_t operator()(const kagome::parachain::BlockedCollationId &value) const {
    using Hash = kagome::parachain::Hash;
    using ParachainId = kagome::parachain::ParachainId;

    size_t result = std::hash<ParachainId>()(value.para_id);
    boost::hash_combine(result, std::hash<Hash>()(value.parent_head_data_hash));
    return result;
  }
};

namespace kagome::parachain {

  class ParachainProcessorEmpty
      : public ParachainStorageImpl,
        public std::enable_shared_from_this<ParachainProcessorEmpty>,
        public BackedCandidatesSource,
        public ParachainProcessor {
    struct Status {
      size_t legacy_statement_counter_ = 0;
      size_t ack_counter_ = 0;
      size_t manifest_counter_ = 0;
      size_t statement_counter_ = 0;
    };

    SafeObject<Status> status_;
    log::Logger logger_ =
        log::createLogger("ParachainProcessorEmpty", "parachain");
    std::shared_ptr<statement_distribution::IStatementDistribution>
        statement_distribution_;

    void process_vstaging_statement(
        const libp2p::peer::PeerId &peer_id,
        const network::vstaging::StatementDistributionMessage &msg);

    void process_legacy_statement(
        const libp2p::peer::PeerId &peer_id,
        const network::StatementDistributionMessage &msg);

   public:
    ParachainProcessorEmpty(
        application::AppStateManager &app_state_manager,
        std::shared_ptr<parachain::AvailabilityStore> av_store,
        std::shared_ptr<statement_distribution::IStatementDistribution>
            statement_distribution);

    bool prepare();

    void onValidationProtocolMsg(
        const libp2p::peer::PeerId &peer_id,
        const network::VersionedValidatorProtocolMessage &message) override;

    void handle_advertisement(const RelayHash &relay_parent,
                              const libp2p::peer::PeerId &peer_id,
                              std::optional<std::pair<CandidateHash, Hash>>
                                  &&prospective_candidate) override;

    void onIncomingCollator(const libp2p::peer::PeerId &peer_id,
                            network::CollatorPublicKey pubkey,
                            network::ParachainId para_id) override;

    outcome::result<void> canProcessParachains() const override;

    void handleStatement(const primitives::BlockHash &relay_parent,
                         const SignedFullStatementWithPVD &statement) override;

    std::vector<network::BackedCandidate> getBackedCandidates(
        const RelayHash &relay_parent) override;
  };

  class ParachainProcessorImpl
      : public ParachainStorageImpl,
        public std::enable_shared_from_this<ParachainProcessorImpl>,
        public BackedCandidatesSource,
        public ParachainProcessor {
   public:
    struct ImportStatementSummary {
      BackingStore::ImportResult imported;
      /// Attested more than threshold
      bool attested;
    };

    enum struct ValidationTaskType { kSecond, kAttest };

    using Commitments = std::shared_ptr<network::CandidateCommitments>;
    struct ValidateAndSecondResult {
      outcome::result<void> result;
      primitives::BlockHash relay_parent;
      Commitments commitments;
      network::CandidateReceipt candidate;
      network::ParachainBlock pov;
      runtime::PersistedValidationData pvd;
    };

    ParachainProcessorImpl(
        std::shared_ptr<network::PeerManager> pm,
        std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
        std::shared_ptr<network::Router> router,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<network::IPeerView> peer_view,
        std::shared_ptr<parachain::IBitfieldSigner> bitfield_signer,
        std::shared_ptr<parachain::IPvfPrecheck> pvf_precheck,
        std::shared_ptr<parachain::BitfieldStore> bitfield_store,
        std::shared_ptr<parachain::BackingStore> backing_store,
        std::shared_ptr<parachain::Pvf> pvf,
        std::shared_ptr<parachain::AvailabilityStore> av_store,
        std::shared_ptr<runtime::ParachainHost> parachain_host,
        std::shared_ptr<parachain::IValidatorSignerFactory> signer_factory,
        const application::AppConfiguration &app_config,
        application::AppStateManager &app_state_manager,
        primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
        primitives::events::SyncStateSubscriptionEnginePtr
            sync_state_observable,
        std::shared_ptr<authority_discovery::Query> query_audi,
        std::shared_ptr<IProspectiveParachains> prospective_parachains,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        LazySPtr<consensus::SlotsUtil> slots_util,
        std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo,
        std::shared_ptr<statement_distribution::IStatementDistribution>
            statement_distribution);
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
     * @ brief We should only process parachains if we are validator and we are
     * @return outcome::result<void> Returns an error if we cannot process the
     * parachains.
     */
    outcome::result<void> canProcessParachains() const override;

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
                            network::ParachainId para_id) override;

    void onValidationProtocolMsg(
        const libp2p::peer::PeerId &peer_id,
        const network::VersionedValidatorProtocolMessage &message) override;

    virtual void OnBroadcastBitfields(const primitives::BlockHash &relay_parent,
                                      const network::SignedBitfield &bitfield);

    virtual void onViewUpdated(const network::ExView &event);

    virtual void handle_collation_fetch_response(
        network::CollationEvent &&collation_event,
        network::CollationFetchingResponse &&response);

    virtual void notifySeconded(const primitives::BlockHash &relay_parent,
                                const SignedFullStatementWithPVD &statement);

    virtual void notifyInvalid(
        const primitives::BlockHash &parent,
        const network::CandidateReceipt &candidate_receipt);

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
    virtual void validateAsync(ValidationTaskType kMode,
                               const network::CandidateReceipt &candidate,
                               const network::ParachainBlock &pov,
                               const runtime::PersistedValidationData &pvd,
                               const primitives::BlockHash &relay_parent);

    /**
     * @brief Handles an incoming advertisement for a collation.
     *
     * @param pending_collation The CollationEvent representing the collation
     * being advertised.
     * @param prospective_candidate An optional pair containing the hash of the
     * prospective candidate and the hash of the parent block.
     */
    void handle_advertisement(const RelayHash &relay_parent,
                              const libp2p::peer::PeerId &peer_id,
                              std::optional<std::pair<CandidateHash, Hash>>
                                  &&prospective_candidate) override;

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
    virtual void makeAvailable(ValidationTaskType kMode,
                               const primitives::BlockHash &candidate_hash,
                               ValidateAndSecondResult &&result);

    virtual void on_pvf_result_received(
        ValidationTaskType kMode,
        size_t n_validators,
        const network::CandidateReceipt &candidate,
        const network::ParachainBlock &pov,
        const runtime::PersistedValidationData &pvd,
        const primitives::BlockHash &relay_parent,
        const Hash &candidate_hash,
        const outcome::result<Pvf::Result> &validation_result);

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

    auto getAvStore() {
      return av_store_;
    }
    auto getBackingStore() {
      return backing_store_;
    }

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
                         const SignedFullStatementWithPVD &statement) override;

   private:
    enum struct StatementType { kSeconded = 0, kValid };
    using WorkersContext = boost::asio::io_context;
    using WorkGuard = boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>;
    using SecondingAllowed = std::optional<std::vector<Hash>>;

    struct AttestingData {
      network::CandidateReceipt candidate;
      primitives::BlockHash pov_hash;
      network::ValidatorIndex from_validator;
      std::queue<network::ValidatorIndex> backing;
    };

    struct TableContext {
      std::optional<std::shared_ptr<IValidatorSigner>> validator;
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

    struct PerSessionState final {
      SessionIndex session;
      runtime::SessionInfo session_info;

      PerSessionState(SessionIndex s, runtime::SessionInfo si)
          : session(s), session_info(std::move(si)) {}
    };

    struct RelayParentState {
      ProspectiveParachainsModeOpt prospective_parachains_mode;
      std::optional<CoreIndex> assigned_core;
      std::vector<std::optional<GroupIndex>> validator_to_group;

      Collations collations;
      TableContext table_context;
      std::vector<runtime::CoreState> availability_cores;
      runtime::GroupDescriptor group_rotation_info;
      uint32_t minimum_backing_votes;
      /// Claim queue state. If the runtime API is not available, it'll be
      /// populated with info from availability cores.
      runtime::ClaimQueueSnapshot claim_queue;

      std::unordered_set<primitives::BlockHash> awaiting_validation;
      std::unordered_set<primitives::BlockHash> issued_statements;
      std::unordered_set<network::PeerId> peers_advertised;
      std::unordered_map<primitives::BlockHash, AttestingData> fallbacks;
      std::unordered_set<CandidateHash> backed_hashes;

      bool inject_core_index;
      bool v2_receipts;
      CoreIndex current_core;
      std::shared_ptr<RefCache<SessionIndex, PerSessionState>::RefObj>
          per_session_state;
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

    outcome::result<consensus::Randomness> getBabeRandomness(
        const RelayHash &relay_parent);

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
    outcome::result<std::optional<std::shared_ptr<IValidatorSigner>>>
    isParachainValidator(const primitives::BlockHash &relay_parent) const;

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
        std::optional<std::pair<std::reference_wrapper<const HeadData>,
                                std::reference_wrapper<const Hash>>>
            maybe_parent_head_and_hash);

    outcome::result<std::optional<runtime::PersistedValidationData>>
    fetchPersistedValidationData(const RelayHash &relay_parent,
                                 ParachainId para_id);
    void onValidationComplete(const ValidateAndSecondResult &result);
    void onAttestComplete(const ValidateAndSecondResult &result);
    void onAttestNoPoVComplete(const network::RelayHash &relay_parent,
                               const CandidateHash &candidate_hash);

    /// Try seconding any collations which were waiting on the validation of
    /// their parent
    void second_unblocked_collations(ParachainId para_id,
                                     const HeadData &head_data,
                                     const Hash &head_data_hash);

    void kickOffValidationWork(
        const RelayHash &relay_parent,
        AttestingData &attesting_data,
        const runtime::PersistedValidationData &persisted_validation_data,
        RelayParentState &parachain_state);
    template <StatementType kStatementType>
    std::optional<network::SignedStatement> createAndSignStatement(
        const ValidateAndSecondResult &validation_result);
    template <ParachainProcessorImpl::StatementType kStatementType>
    outcome::result<std::optional<SignedFullStatementWithPVD>>
    sign_import_and_distribute_statement(
        ParachainProcessorImpl::RelayParentState &rp_state,
        const ValidateAndSecondResult &validation_result);
    void post_import_statement_actions(
        const RelayHash &relay_parent,
        ParachainProcessorImpl::RelayParentState &rp_state,
        std::optional<BackingStore::ImportResult> &summary);
    template <typename T>
    std::optional<network::SignedStatement> createAndSignStatementFromPayload(
        T &&payload, RelayParentState &parachain_state);
    outcome::result<std::optional<BackingStore::ImportResult>> importStatement(
        const network::RelayHash &relay_parent,
        const SignedFullStatementWithPVD &statement,
        ParachainProcessorImpl::RelayParentState &relayParentState);
    std::optional<CoreIndex> core_index_from_statement(
        const std::vector<std::optional<GroupIndex>> &validator_to_group,
        const runtime::GroupDescriptor &group_rotation_info,
        uint32_t n_cores,
        const SignedFullStatementWithPVD &statement,
        const runtime::ClaimQueueSnapshot &claim_queue);
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

    /*
     * Notification
     */
    void notifyAvailableData(std::vector<network::ErasureChunk> &&chunk_list,
                             const primitives::BlockHash &relay_parent,
                             const network::CandidateHash &candidate_hash,
                             const network::ParachainBlock &pov,
                             const runtime::PersistedValidationData &data);

    /// Notify a collator that its collation got seconded.
    void notify_collation_seconded(const libp2p::peer::PeerId &peer_id,
                                   network::CollationVersion version,
                                   const RelayHash &relay_parent,
                                   const SignedFullStatementWithPVD &statement);

    void handle_active_leaves_update_for_validator(const network::ExView &event,
                                                   std::vector<Hash> pruned);
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

    void prune_old_advertisements(
        const parachain::ImplicitView &implicit_view,
        const std::unordered_map<Hash, ProspectiveParachainsModeOpt>
            &active_leaves,
        const std::unordered_map<primitives::BlockHash, RelayParentState>
            &per_relay_parent);

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
    std::vector<Hash> create_backing_task(
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

   private:
    outcome::result<void> enqueueCollation(
        const RelayHash &relay_parent,
        ParachainId para_id,
        const libp2p::peer::PeerId &peer_id,
        const CollatorId &collator_id,
        std::optional<std::pair<CandidateHash, Hash>> &&prospective_candidate);

    bool isValidatingNode() const;
    bool canSecond(ParachainId candidate_para_id,
                   const Hash &candidate_relay_parent,
                   const CandidateHash &candidate_hash,
                   const Hash &parent_head_data_hash);

    ParachainProcessorImpl::SecondingAllowed seconding_sanity_check(
        const HypotheticalCandidate &hypothetical_candidate);

    void printStoragesLoad();

    /// Handle a fetched collation result.
    /// polkadot/node/network/collator-protocol/src/validator_side/mod.rs
    /// Returns `true` if seconding started.
    outcome::result<bool> kick_off_seconding(
        network::PendingCollationFetch &&pending_collation_fetch);

    std::optional<BackingStore::ImportResult> importStatementToTable(
        const RelayHash &relay_parent,
        ParachainProcessorImpl::RelayParentState &relayParentState,
        GroupIndex group_id,
        const primitives::BlockHash &candidate_hash,
        const network::SignedStatement &statement);

    std::shared_ptr<network::PeerManager> pm_;
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
      std::unordered_set<PendingCollation,
                         PendingCollationHash,
                         PendingCollationEq>
          collation_requests_cancel_handles;

      struct {
        std::unordered_map<Hash, ProspectiveParachainsModeOpt> active_leaves;
        std::unordered_map<network::FetchedCollation, network::CollationEvent>
            fetched_candidates;
        std::unordered_map<BlockedCollationId,
                           std::vector<network::PendingCollationFetch>>
            blocked_from_seconding;
      } validator_side;
    } our_current_state_;

    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<network::IPeerView> peer_view_;
    network::IPeerView::MyViewSubscriberPtr my_view_sub_;

    std::shared_ptr<parachain::Pvf> pvf_;
    std::shared_ptr<parachain::IValidatorSignerFactory> signer_factory_;
    std::shared_ptr<parachain::IBitfieldSigner> bitfield_signer_;
    std::shared_ptr<parachain::IPvfPrecheck> pvf_precheck_;
    std::shared_ptr<parachain::BitfieldStore> bitfield_store_;
    std::shared_ptr<parachain::BackingStore> backing_store_;
    std::shared_ptr<runtime::ParachainHost> parachain_host_;
    const application::AppConfiguration &app_config_;
    primitives::events::SyncStateSubscriptionEnginePtr sync_state_observable_;
    std::shared_ptr<void> sync_state_observer_;
    std::shared_ptr<authority_discovery::Query> query_audi_;
    LazySPtr<consensus::SlotsUtil> slots_util_;
    std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo_;

    bool synchronized_ = false;
    primitives::events::ChainSub chain_sub_;
    std::default_random_engine random_;
    std::shared_ptr<IProspectiveParachains> prospective_parachains_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<statement_distribution::IStatementDistribution>
        statement_distribution;
    std::shared_ptr<RefCache<SessionIndex, PerSessionState>> per_session;

    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_is_parachain_validator_;
    std::unordered_map<Hash, BlockNumber> existed_leaves_;
    metrics::Counter
        *metric_kagome_parachain_candidate_backing_signed_statements_total_;
    metrics::Counter
        *metric_kagome_parachain_candidate_backing_candidates_seconded_total_;

   public:
    void handle_second_message(const network::CandidateReceipt &candidate,
                               const network::ParachainBlock &pov,
                               const runtime::PersistedValidationData &pvd,
                               const primitives::BlockHash &relay_parent) {
      validateAsync(ValidationTaskType::kSecond,
                    candidate,
                    network::ParachainBlock(pov),
                    runtime::PersistedValidationData(pvd),
                    relay_parent);
    }
  };

  class ThreadedParachainProcessorImpl : public ParachainProcessorImpl {
    std::shared_ptr<PoolHandler> main_pool_handler_;

   public:
    ThreadedParachainProcessorImpl(
        application::AppStateManager &app_state_manager,
        common::MainThreadPool &main_thread_pool,
        std::shared_ptr<network::PeerManager> pm,
        std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
        std::shared_ptr<network::Router> router,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<network::IPeerView> peer_view,
        std::shared_ptr<parachain::IBitfieldSigner> bitfield_signer,
        std::shared_ptr<parachain::IPvfPrecheck> pvf_precheck,
        std::shared_ptr<parachain::BitfieldStore> bitfield_store,
        std::shared_ptr<parachain::BackingStore> backing_store,
        std::shared_ptr<parachain::Pvf> pvf,
        std::shared_ptr<parachain::AvailabilityStore> av_store,
        std::shared_ptr<runtime::ParachainHost> parachain_host,
        std::shared_ptr<parachain::IValidatorSignerFactory> signer_factory,
        const application::AppConfiguration &app_config,
        primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
        primitives::events::SyncStateSubscriptionEnginePtr
            sync_state_observable,
        std::shared_ptr<authority_discovery::Query> query_audi,
        std::shared_ptr<IProspectiveParachains> prospective_parachains,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        LazySPtr<consensus::SlotsUtil> slots_util,
        std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo,
        std::shared_ptr<statement_distribution::IStatementDistribution>
            statement_distribution)
        : ParachainProcessorImpl(std::move(pm),
                                 std::move(crypto_provider),
                                 std::move(router),
                                 std::move(hasher),
                                 std::move(peer_view),
                                 std::move(bitfield_signer),
                                 std::move(pvf_precheck),
                                 std::move(bitfield_store),
                                 std::move(backing_store),
                                 std::move(pvf),
                                 std::move(av_store),
                                 std::move(parachain_host),
                                 std::move(signer_factory),
                                 app_config,
                                 app_state_manager,
                                 std::move(chain_sub_engine),
                                 std::move(sync_state_observable),
                                 std::move(query_audi),
                                 std::move(prospective_parachains),
                                 std::move(block_tree),
                                 slots_util,
                                 std::move(babe_config_repo),
                                 std::move(statement_distribution)),
          main_pool_handler_{main_thread_pool.handler(app_state_manager)} {
    }

    void onValidationProtocolMsg(
        const libp2p::peer::PeerId &peer_id,
        const network::VersionedValidatorProtocolMessage &message) override {
      REINVOKE(*main_pool_handler_,
               onValidationProtocolMsg,
               peer_id,
               message);
      ParachainProcessorImpl::onValidationProtocolMsg(peer_id, message);
    }

    void OnBroadcastBitfields(
        const primitives::BlockHash &relay_parent,
        const network::SignedBitfield &bitfield) override {
      REINVOKE(*main_pool_handler_,
               OnBroadcastBitfields,
               relay_parent,
               bitfield);
      ParachainProcessorImpl::OnBroadcastBitfields(relay_parent, bitfield);
    }

    void onViewUpdated(const network::ExView &event) override {
      REINVOKE(
          *main_pool_handler_, onViewUpdated, event);
      ParachainProcessorImpl::onViewUpdated(event);
    }

    void handle_collation_fetch_response(
        network::CollationEvent &&collation_event,
        network::CollationFetchingResponse &&response) override {
      REINVOKE(*main_pool_handler_,
               handle_collation_fetch_response,
               std::move(collation_event),
               std::move(response));
      ParachainProcessorImpl::handle_collation_fetch_response(std::move(collation_event), std::move(response));
    }

    void handleStatement(const primitives::BlockHash &relay_parent,
                         const SignedFullStatementWithPVD &statement) override {
      REINVOKE(*main_pool_handler_,
               handleStatement,
               relay_parent,
               statement);
      ParachainProcessorImpl::handleStatement(relay_parent, statement);
    }

    void notifyInvalid(
        const primitives::BlockHash &parent,
        const network::CandidateReceipt &candidate_receipt) override {
      REINVOKE_ONCE(*main_pool_handler_,
                    ParachainProcessorImpl::notifyInvalid,
                    parent,
                    candidate_receipt);
    }

    void notifySeconded(const primitives::BlockHash &parent,
                        const SignedFullStatementWithPVD &statement) override {
      REINVOKE_ONCE(*main_pool_handler_,
                    ParachainProcessorImpl::notifySeconded,
                    parent,
                    statement);
    }

    void makeAvailable(
        ValidationTaskType kMode,
        const primitives::BlockHash &candidate_hash,
        ValidateAndSecondResult &&validate_and_second_result) override {
      REINVOKE(*main_pool_handler_,
               makeAvailable,
               kMode,
               candidate_hash,
               std::move(validate_and_second_result));
      ParachainProcessorImpl::makeAvailable(kMode,
               candidate_hash,
               std::move(validate_and_second_result));
    }

    void validateAsync(ValidationTaskType kMode,
                       const network::CandidateReceipt &candidate,
                       const network::ParachainBlock &pov,
                       const runtime::PersistedValidationData &pvd,
                       const primitives::BlockHash &relay_parent) override {
      REINVOKE(*main_pool_handler_,
               validateAsync,
               kMode,
               candidate,
               pov,
               pvd,
               relay_parent);
      ParachainProcessorImpl::validateAsync(kMode,
               candidate,
               pov,
               pvd,
               relay_parent);
    }

    void on_pvf_result_received(
        ValidationTaskType kMode,
        size_t n_validators,
        const network::CandidateReceipt &candidate,
        const network::ParachainBlock &pov,
        const runtime::PersistedValidationData &pvd,
        const primitives::BlockHash &relay_parent,
        const Hash &candidate_hash,
        const outcome::result<Pvf::Result> &validation_result) override {
      REINVOKE_ONCE(*main_pool_handler_,
                    ParachainProcessorImpl::on_pvf_result_received,
                    kMode,
                    n_validators,
                    candidate,
                    pov,
                    pvd,
                    relay_parent,
                    candidate_hash,
                    validation_result);
    }

    void handle_advertisement(const RelayHash &relay_parent,
                              const libp2p::peer::PeerId &peer_id,
                              std::optional<std::pair<CandidateHash, Hash>>
                                  &&prospective_candidate) override {
      REINVOKE(*main_pool_handler_,
               handle_advertisement,
               relay_parent,
               peer_id,
               std::move(prospective_candidate));
      ParachainProcessorImpl::handle_advertisement(relay_parent,
               peer_id,
               std::move(prospective_candidate));
    }
  };

}  // namespace kagome::parachain
