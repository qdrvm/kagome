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
#include "network/peer_view.hpp"
#include "network/protocols/req_collation_protocol.hpp"
#include "network/types/collator_messages.hpp"
#include "outcome/outcome.hpp"
#include "parachain/thread_pool.hpp"
#include "primitives/common.hpp"
#include "primitives/event_types.hpp"
#include "utils/non_copyable.hpp"
#include "utils/safe_object.hpp"

namespace kagome::network {
  class PeerManager;
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
      VALIDATION_SKIPPED
    };
    static constexpr uint64_t kBackgroundWorkers = 5;

    struct ImportStatementSummary {
      /// The digest of the candidate.
      primitives::BlockHash candidate;
      /// The group that the candidate is in.
      network::ParachainId group_id;
      /// How many validity votes are currently witnessed.
      uint64_t validity_votes;
      /// Attested more than threshold
      bool attested;
    };

    ParachainProcessorImpl(
        std::shared_ptr<network::PeerManager> pm,
        std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
        std::shared_ptr<network::Router> router,
        std::shared_ptr<boost::asio::io_context> this_context,
        std::shared_ptr<crypto::Sr25519Keypair> keypair,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<network::PeerView> peer_view,
        std::shared_ptr<thread::ThreadPool> thread_pool);
    ~ParachainProcessorImpl() = default;

    bool start();
    void stop();
    bool prepare();
    void requestCollations(network::PendingCollation const &pending_collation);
    void setAssignedParachain(std::optional<network::ParachainId> para_id,
                              const primitives::BlockHash &relay_parent);
    void handleStatement(
        libp2p::peer::PeerId const &peer_id,
        primitives::BlockHash const &relay_parent,
        std::shared_ptr<network::SignedStatement> const &statement);
    void onIncomingCollator(libp2p::peer::PeerId const &peer_id,
                            network::CollatorPublicKey pubkey,
                            network::ParachainId para_id);
    void onIncomingCollationStream(libp2p::peer::PeerId const &peer_id);
    void onIncomingValidationStream(libp2p::peer::PeerId const &peer_id);

   private:
    enum struct CollationState { kFetched, kSeconded };
    enum struct PoVDataState { kComplete, kFetchFromValidator };
    enum struct StatementType { kSeconded = 0, kValid };

    struct FetchedCollationState {
      network::CollationFetchingResponse fetched_collation;
      CollationState collation_state;
      PoVDataState pov_state;

      FetchedCollationState(network::CollationFetchingResponse &&fc,
                            CollationState cs,
                            PoVDataState ps)
          : fetched_collation(std::move(fc)),
            collation_state(cs),
            pov_state(ps) {}
    };

    using FetchedCollation = std::shared_ptr<FetchedCollationState>;
    using Commitments = std::shared_ptr<network::CandidateCommitments>;
    using PeerMap = std::unordered_map<libp2p::peer::PeerId, FetchedCollation>;
    using RelayParentMap = std::unordered_map<primitives::BlockHash, PeerMap>;
    using ParachainMap =
        std::unordered_map<network::ParachainId, RelayParentMap>;
    using WorkersContext = boost::asio::io_context;
    using WorkGuard = boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>;

    struct ValidateAndSecondResult {
      outcome::result<void> result;
      FetchedCollation fetched_collation;
      primitives::BlockHash relay_parent;
      Commitments commitments;
    };

    struct AttestingData {
      std::shared_ptr<network::SignedStatement> statement;
      primitives::BlockHash candidate_hash;
      primitives::BlockHash pov_hash;
      network::ValidatorIndex from_validator;
      std::queue<network::ValidatorIndex> backing;
    };

    struct RelayParentState {
      std::optional<network::ParachainId> assignment;
      std::optional<primitives::BlockHash> seconded;
      std::optional<network::ValidatorIndex> our_index;

      std::unordered_set<primitives::BlockHash> awaiting_validation;
      std::unordered_set<primitives::BlockHash> issued_statements;
      std::unordered_map<primitives::BlockHash, AttestingData> fallbacks;
    };

    /*
     * Validation.
     */
    outcome::result<void> validateCandidate(
        FetchedCollation &fetched_collation);
    outcome::result<void> validateErasureCoding(
        FetchedCollation &fetched_collation);
    outcome::result<ValidateAndSecondResult> validateAndMakeAvailable(
        std::weak_ptr<ParachainProcessorImpl> wptr,
        libp2p::peer::PeerId const &peer_id,
        primitives::BlockHash const &relay_parent,
        FetchedCollation fetched_collation);
    void requestPoV();

    /*
     * Logic.
     */
    void onValidationComplete(libp2p::peer::PeerId const &peer_id,
                              ValidateAndSecondResult &&result);
    void onAttestComplete(libp2p::peer::PeerId const &peer_id,
                          ValidateAndSecondResult &&result);
    void onAttestNoPoVComplete(libp2p::peer::PeerId const &peer_id,
                               ValidateAndSecondResult &&result);
    void appendAsyncValidationTask(network::ParachainId id,
                                   primitives::BlockHash const &relay_parent,
                                   libp2p::peer::PeerId const &peer_id,
                                   RelayParentState &parachain_state,
                                   const primitives::BlockHash &candidate_hash);
    void kickOffValidationWork(AttestingData &attesting_data,
                               RelayParentState &parachain_state);
    void handleFetchedCollation(network::ParachainId id,
                                primitives::BlockHash const &relay_parent,
                                libp2p::peer::PeerId const &peer_id,
                                network::CollationFetchingResponse &&response);
    template <StatementType kStatementType>
    std::shared_ptr<network::SignedStatement> createAndSignStatement(
        ValidateAndSecondResult &validation_result);
    template <typename T>
    std::shared_ptr<network::SignedStatement> createAndSignStatementFromPayload(
        T &&payload, network::ValidatorIndex our_ix);
    std::optional<ImportStatementSummary> importStatement(
        std::shared_ptr<network::SignedStatement> const &statement);

    /*
     * Helpers.
     */
    primitives::BlockHash candidateHashFrom(
        FetchedCollationState const &fetched_collation_state) {
      return visit_in_place(
          fetched_collation_state.fetched_collation.response_data,
          [&](network::CollationResponse const &collation_response)
              -> primitives::BlockHash {
            return hasher_->blake2b_256(
                scale::encode(collation_response.receipt).value());
          });
    }

    network::CandidateDescriptor const &candidateDescriptorFrom(
        FetchedCollationState const &fetched_collation_state) {
      return visit_in_place(
          fetched_collation_state.fetched_collation.response_data,
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
                scale::encode(network::CandidateReceipt{
                                  .descriptor = data.descriptor,
                                  .commitments_hash = hasher_->blake2b_256(
                                      scale::encode(data.commitments).value())})
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
    void notifyBackedCandidate(
        std::shared_ptr<network::SignedStatement> const &statement);
    void notifyAvailableData();
    void notifyStatementDistributionSystem(
        primitives::BlockHash const &relay_parent,
        std::shared_ptr<network::SignedStatement> const &statement);
    void notify(libp2p::peer::PeerId const &peer_id,
                primitives::BlockHash const &relay_parent,
                std::shared_ptr<network::SignedStatement> const &statement);
    void handleNotify(libp2p::peer::PeerId const &peer_id,
                      primitives::BlockHash const &relay_parent);

    outcome::result<FetchedCollation> loadIntoResponseSlot(
        network::ParachainId id,
        primitives::BlockHash const &relay_parent,
        libp2p::peer::PeerId const &peer_id,
        network::CollationFetchingResponse &&response);

    outcome::result<FetchedCollation> getFromSlot(
        network::ParachainId id,
        primitives::BlockHash const &relay_parent,
        libp2p::peer::PeerId const &peer_id);

    std::optional<std::reference_wrapper<RelayParentState>>
    tryGetStateByRelayParent(const primitives::BlockHash &relay_parent);
    RelayParentState &getStateByRelayParent(
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
        primitives::BlockHash const &candidate_hash,
        std::shared_ptr<network::SignedStatement> const &statement);

    std::shared_ptr<network::PeerManager> pm_;
    std::shared_ptr<crypto::Sr25519Provider> crypto_provider_;
    std::shared_ptr<network::Router> router_;
    log::Logger logger_ =
        log::createLogger("ParachainProcessorImpl", "parachain");

    struct {
      std::optional<network::CollatorPublicKey> required_collator;
      std::unordered_map<primitives::BlockHash, RelayParentState>
          state_by_relay_parent;
      std::unordered_map<libp2p::peer::PeerId,
                         std::deque<std::shared_ptr<network::SignedStatement>>>
          seconded_statements;
    } our_current_state_;

    SafeObject<ParachainMap> collations_;
    std::shared_ptr<WorkersContext> this_context_;
    std::shared_ptr<crypto::Sr25519Keypair> keypair_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<network::PeerView> peer_view_;
    network::PeerView::MyViewSubscriberPtr my_view_sub_;
    std::shared_ptr<thread::ThreadPool> thread_pool_;
  };

}  // namespace kagome::parachain

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain, ParachainProcessorImpl::Error);

#endif  // KAGOME_PARACHAIN_PROCESSOR_HPP
