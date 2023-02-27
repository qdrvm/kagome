/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_PROCESSOR_HPP
#define KAGOME_PARACHAIN_PROCESSOR_HPP

#include <memory>
#include <queue>
#include <thread>
#include <unordered_map>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/signal_set.hpp>
#include <libp2p/peer/peer_id.hpp>

#include "common/visitor.hpp"
#include "crypto/hasher.hpp"
#include "network/peer_manager.hpp"
#include "network/peer_view.hpp"
#include "network/protocols/req_collation_protocol.hpp"
#include "network/types/collator_messages.hpp"
#include "outcome/outcome.hpp"
#include "parachain/availability/bitfield/signer.hpp"
#include "parachain/availability/store/store.hpp"
#include "parachain/backing/store.hpp"
#include "parachain/pvf/pvf.hpp"
#include "parachain/validator/signer.hpp"
#include "primitives/common.hpp"
#include "primitives/event_types.hpp"
#include "utils/non_copyable.hpp"
#include "utils/safe_object.hpp"
#include "utils/thread_pool.hpp"

namespace kagome::network {
  class Router;
  struct PendingCollation;
}  // namespace kagome::network

namespace kagome::crypto {
  class Sr25519Provider;
  class Hasher;
}  // namespace kagome::crypto

namespace kagome::parachain {

  struct ParachainProcessorImpl
      : std::enable_shared_from_this<ParachainProcessorImpl> {
    enum class Error {
      RESPONSE_ALREADY_RECEIVED = 1,
      COLLATION_NOT_FOUND,
      KEY_NOT_PRESENT,
      VALIDATION_FAILED,
      VALIDATION_SKIPPED,
      OUT_OF_VIEW,
      DUPLICATE,
      NO_INSTANCE,
      NOT_A_VALIDATOR
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
        std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
        std::shared_ptr<network::Router> router,
        std::shared_ptr<boost::asio::io_context> this_context,
        std::shared_ptr<crypto::Sr25519Keypair> keypair,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<network::PeerView> peer_view,
        std::shared_ptr<ThreadPool> thread_pool,
        std::shared_ptr<parachain::BitfieldSigner> bitfield_signer,
        std::shared_ptr<parachain::BitfieldStore> bitfield_store,
        std::shared_ptr<parachain::BackingStore> backing_store,
        std::shared_ptr<parachain::Pvf> pvf,
        std::shared_ptr<parachain::AvailabilityStore> av_store,
        std::shared_ptr<runtime::ParachainHost> parachain_host,
        std::shared_ptr<parachain::ValidatorSignerFactory> signer_factory);
    ~ParachainProcessorImpl() = default;

    bool start();
    void stop();
    bool prepare();
    void requestCollations(network::CollationEvent const &pending_collation);
    outcome::result<void> advCanBeProcessed(
        primitives::BlockHash const &relay_parent,
        libp2p::peer::PeerId const &peer_id);

    void handleStatement(libp2p::peer::PeerId const &peer_id,
                         primitives::BlockHash const &relay_parent,
                         network::SignedStatement const &statement);
    void onIncomingCollator(libp2p::peer::PeerId const &peer_id,
                            network::CollatorPublicKey pubkey,
                            network::ParachainId para_id);
    void onIncomingCollationStream(libp2p::peer::PeerId const &peer_id);
    void onIncomingValidationStream(libp2p::peer::PeerId const &peer_id);
    void onValidationProtocolMsg(
        libp2p::peer::PeerId const &peer_id,
        network::ValidatorProtocolMessage const &message);
    outcome::result<network::FetchChunkResponse> OnFetchChunkRequest(
        network::FetchChunkRequest const &request);
    template <typename F>
    void recoverCandidate(F &&complete_callback) {
      /// TODO(iceseer): not implemented
    }

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

    struct ValidateAndSecondResult {
      outcome::result<void> result;
      primitives::BlockHash relay_parent;
      Commitments commitments;
      network::CandidateReceipt candidate;
      network::ParachainBlock pov;
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

      size_t requisite_votes(ParachainId group) const {
        if (auto it = groups.find(group); it != groups.end()) {
          return minimum_votes(it->second.size());
        }
        return std::numeric_limits<size_t>::max();
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

    struct RelayParentState {
      std::optional<network::ParachainId> assignment;
      std::optional<primitives::BlockHash> seconded;
      std::optional<network::ValidatorIndex> our_index;
      std::optional<network::CollatorPublicKey> required_collator;

      TableContext table_context;

      std::unordered_set<primitives::BlockHash> awaiting_validation;
      std::unordered_set<primitives::BlockHash> issued_statements;
      std::unordered_set<network::PeerId> peers_advertised;
      std::unordered_map<primitives::BlockHash, AttestingData> fallbacks;
    };

    struct AvailableDataRef {
      SCALE_TIE(2);

      /// The Proof-of-Validation of the candidate.
      network::ParachainBlock const &pov;
      /// The persisted validation data needed for secondary checks.
      runtime::PersistedValidationData const &validation_data;
    };

    /*
     * Validation.
     */
    outcome::result<Pvf::Result> validateCandidate(
        network::CandidateReceipt const &candidate,
        network::ParachainBlock const &pov);
    template <typename T>
    outcome::result<T> validateErasureCoding(AvailableDataRef &&validating_data,
                                             size_t n_validators);
    outcome::result<ValidateAndSecondResult> validateAndMakeAvailable(
        network::CandidateReceipt &&candidate,
        network::ParachainBlock &&pov,
        libp2p::peer::PeerId const &peer_id,
        primitives::BlockHash const &relay_parent,
        size_t n_validators);
    template <typename F>
    void requestPoV(libp2p::peer::PeerId const &peer_id,
                    CandidateHash const &candidate_hash,
                    F &&callback);

    std::optional<AttestedCandidate> attested_candidate(
        CandidateHash const &digest, TableContext const &context);

    std::optional<AttestedCandidate> attested(
        network::CommittedCandidateReceipt &&candidate,
        BackingStore::StatementInfo const &data,
        size_t validity_threshold);
    std::optional<BackingStore::BackedCandidate> table_attested_to_backed(
        AttestedCandidate &&attested, TableContext &table_context);

    /*
     * Logic.
     */
    void onValidationComplete(libp2p::peer::PeerId const &peer_id,
                              ValidateAndSecondResult &&result);
    void onAttestComplete(libp2p::peer::PeerId const &peer_id,
                          ValidateAndSecondResult &&result);
    void onAttestNoPoVComplete(libp2p::peer::PeerId const &peer_id,
                               ValidateAndSecondResult &&result);

    template <ValidationTaskType kMode>
    void appendAsyncValidationTask(network::CandidateReceipt &&candidate,
                                   network::ParachainBlock &&pov,
                                   primitives::BlockHash const &relay_parent,
                                   libp2p::peer::PeerId const &peer_id,
                                   RelayParentState &parachain_state,
                                   const primitives::BlockHash &candidate_hash,
                                   size_t n_validators);
    void kickOffValidationWork(RelayHash const &relay_parent,
                               libp2p::peer::PeerId const &peer_id,
                               AttestingData &attesting_data,
                               RelayParentState &parachain_state);
    void handleFetchedCollation(network::CollationEvent &&pending_collation,
                                network::CollationFetchingResponse &&response);
    template <StatementType kStatementType>
    std::optional<network::SignedStatement> createAndSignStatement(
        ValidateAndSecondResult &validation_result);
    template <typename T>
    std::optional<network::SignedStatement> createAndSignStatementFromPayload(
        T &&payload,
        ValidatorIndex validator_ix,
        RelayParentState &parachain_state);
    std::optional<ImportStatementSummary> importStatement(
        network::RelayHash const &relay_parent,
        network::SignedStatement const &statement,
        ParachainProcessorImpl::RelayParentState &relayParentState);

    /*
     * Helpers.
     */
    primitives::BlockHash candidateHashFrom(
        network::CandidateReceipt const &candidate) {
      return hasher_->blake2b_256(scale::encode(candidate).value());
    }
    primitives::BlockHash candidateHashFrom(
        network::CollationFetchingResponse const &collation) {
      return visit_in_place(
          collation.response_data,
          [&](network::CollationResponse const &collation_response)
              -> primitives::BlockHash {
            return candidateHashFrom(collation_response.receipt);
          });
    }

    network::CandidateDescriptor const &candidateDescriptorFrom(
        network::CollationFetchingResponse const &collation) {
      return visit_in_place(
          collation.response_data,
          [](network::CollationResponse const &collation_response)
              -> network::CandidateDescriptor const & {
            return collation_response.receipt.descriptor;
          });
    }

    std::optional<std::reference_wrapper<network::CandidateDescriptor const>>
    candidateDescriptorFrom(network::Statement const &statement) {
      return visit_in_place(
          statement.candidate_state,
          [](network::CommittedCandidateReceipt const &receipt)
              -> std::optional<
                  std::reference_wrapper<network::CandidateDescriptor const>> {
            return receipt.descriptor;
          },
          [](...)
              -> std::optional<
                  std::reference_wrapper<network::CandidateDescriptor const>> {
            BOOST_ASSERT(false);
            return std::nullopt;
          });
    }

    network::CollatorPublicKey const &collatorIdFromDescriptor(
        network::CandidateDescriptor const &descriptor) {
      return descriptor.collator_id;
    }

    network::CandidateReceipt candidateFromCommittedCandidateReceipt(
        network::CommittedCandidateReceipt const &data) {
      return network::CandidateReceipt{
          .descriptor = data.descriptor,
          .commitments_hash =
              hasher_->blake2b_256(scale::encode(data.commitments).value())};
    }

    primitives::BlockHash candidateHashFrom(
        network::Statement const &statement) {
      return visit_in_place(
          statement.candidate_state,
          [](network::Dummy const &) {
            BOOST_ASSERT(!"Not used!");
            return primitives::BlockHash{};
          },
          [&](network::CommittedCandidateReceipt const &data) {
            return hasher_->blake2b_256(
                scale::encode(candidateFromCommittedCandidateReceipt(data))
                    .value());
          },
          [&](primitives::BlockHash const &candidate_hash) {
            return candidate_hash;
          });
    }

    /*
     * Notification
     */
    template <typename F>
    void notify_internal(std::shared_ptr<WorkersContext> &context, F &&func) {
      BOOST_ASSERT(context);
      boost::asio::post(*context, std::forward<F>(func));
    }
    void notifyBackedCandidate(network::SignedStatement const &statement);
    template <typename T>
    void notifyAvailableData(T &chunk_list,
                             network::CandidateHash const &candidate_hash,
                             network::ParachainBlock const &pov,
                             runtime::PersistedValidationData const &data);
    void notifyStatementDistributionSystem(
        primitives::BlockHash const &relay_parent,
        network::SignedStatement const &statement);
    void notify(libp2p::peer::PeerId const &peer_id,
                primitives::BlockHash const &relay_parent,
                network::SignedStatement const &statement);
    void handleNotify(libp2p::peer::PeerId const &peer_id,
                      primitives::BlockHash const &relay_parent);

    std::optional<std::reference_wrapper<RelayParentState>>
    tryGetStateByRelayParent(const primitives::BlockHash &relay_parent);
    RelayParentState &storeStateByRelayParent(
        const primitives::BlockHash &relay_parent, RelayParentState &&val);

    void createBackingTask(const primitives::BlockHash &relay_parent);
    outcome::result<RelayParentState> initNewBackingTask(
        const primitives::BlockHash &relay_parent);

    template <typename F>
    bool tryOpenOutgoingCollatingStream(libp2p::peer::PeerId const &peer_id,
                                        F &&callback);
    template <typename F>
    bool tryOpenOutgoingValidationStream(libp2p::peer::PeerId const &peer_id,
                                         F &&callback);
    template <typename F>
    bool tryOpenOutgoingStream(libp2p::peer::PeerId const &peer_id,
                               std::shared_ptr<network::ProtocolBase> protocol,
                               F &&callback);

    void sendMyView(const libp2p::peer::PeerId &peer_id,
                    const std::shared_ptr<network::Stream> &stream,
                    const std::shared_ptr<network::ProtocolBase> &protocol);

    template <typename T>
    outcome::result<network::Signature> sign(T const &t) const;

    std::optional<ImportStatementSummary> importStatementToTable(
        ParachainProcessorImpl::RelayParentState &relayParentState,
        primitives::BlockHash const &candidate_hash,
        network::SignedStatement const &statement);

    std::shared_ptr<network::PeerManager> pm_;
    std::shared_ptr<crypto::Sr25519Provider> crypto_provider_;
    std::shared_ptr<network::Router> router_;
    log::Logger logger_ =
        log::createLogger("ParachainProcessorImpl", "parachain");

    struct {
      std::unordered_map<primitives::BlockHash, RelayParentState>
          state_by_relay_parent;
      std::unordered_map<
          libp2p::peer::PeerId,
          std::deque<std::pair<RelayHash, network::SignedStatement>>>
          seconded_statements;
    } our_current_state_;

    SafeObject<std::unordered_map<RelayHash, network::CollationEvent>>
        pending_candidates;
    std::shared_ptr<WorkersContext> this_context_;
    std::shared_ptr<crypto::Sr25519Keypair> keypair_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<network::PeerView> peer_view_;
    network::PeerView::MyViewSubscriberPtr my_view_sub_;
    std::shared_ptr<ThreadPool> thread_pool_;

    std::shared_ptr<parachain::Pvf> pvf_;
    std::shared_ptr<parachain::ValidatorSignerFactory> signer_factory_;
    std::shared_ptr<parachain::BitfieldSigner> bitfield_signer_;
    std::shared_ptr<parachain::BitfieldStore> bitfield_store_;
    std::shared_ptr<parachain::BackingStore> backing_store_;
    std::shared_ptr<parachain::AvailabilityStore> av_store_;
    std::shared_ptr<runtime::ParachainHost> parachain_host_;
  };

}  // namespace kagome::parachain

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain, ParachainProcessorImpl::Error);

#endif  // KAGOME_PARACHAIN_PROCESSOR_HPP
