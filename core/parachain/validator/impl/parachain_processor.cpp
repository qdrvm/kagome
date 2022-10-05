/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/parachain_processor.hpp"

#include <gsl/span>

#include "crypto/hasher.hpp"
#include "crypto/sr25519_provider.hpp"
#include "network/common.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/impl/protocols/protocol_error.hpp"
#include "network/peer_manager.hpp"
#include "network/router.hpp"
#include "scale/scale.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain,
                            ParachainProcessorImpl::Error,
                            e) {
  using E = kagome::parachain::ParachainProcessorImpl::Error;
  switch (e) {
    case E::RESPONSE_ALREADY_RECEIVED:
      return "Response already present";
    case E::COLLATION_NOT_FOUND:
      return "Collation not found";
    case E::KEY_NOT_PRESENT:
      return "Private key is not present";
  }
  return "Unknown parachain processor error";
}

namespace kagome::parachain {

  ParachainProcessorImpl::ParachainProcessorImpl(
      std::shared_ptr<network::PeerManager> pm,
      std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
      std::shared_ptr<network::Router> router,
      std::shared_ptr<boost::asio::io_context> this_context,
      std::shared_ptr<crypto::Sr25519Keypair> keypair,
      std::shared_ptr<crypto::Hasher> hasher)
      : pm_(std::move(pm)),
        crypto_provider_(std::move(crypto_provider)),
        router_(std::move(router)),
        this_context_(std::move(this_context)),
        keypair_(std::move(keypair)),
        hasher_(std::move(hasher)) {
    BOOST_ASSERT(pm_);
    BOOST_ASSERT(crypto_provider_);
    BOOST_ASSERT(this_context_);
    BOOST_ASSERT(router_);
    BOOST_ASSERT(hasher_);
  }

  ParachainProcessorImpl::~ParachainProcessorImpl() {
    /// check that all workers are stopped.
#ifndef NDEBUG
    for (auto &w : workers_) {
      BOOST_ASSERT(!w);
    }
#endif  // NDEBUG
  }

  bool ParachainProcessorImpl::prepare() {
    context_ = std::make_shared<WorkersContext>();
    work_guard_ = std::make_shared<WorkGuard>(context_->get_executor());
    return true;
  }

  bool ParachainProcessorImpl::start() {
    BOOST_ASSERT(context_);
    BOOST_ASSERT(work_guard_);
    for (auto &worker : workers_)
      if (!worker)
        worker = std::make_unique<std::thread>(
            [wptr{weak_from_this()}, context{context_}]() {
              if (auto self = wptr.lock())
                self->logger_->debug("Started parachain worker with id: {}",
                                     std::this_thread::get_id());

              context->run();
            });
    return true;
  }

  void ParachainProcessorImpl::stop() {
    work_guard_.reset();
    if (context_) {
      context_->stop();
      context_.reset();
    }
    for (auto &worker : workers_) {
      if (worker) {
        BOOST_ASSERT(worker->joinable());
        worker->join();
        worker.reset();
      }
    }
  }

  outcome::result<ParachainProcessorImpl::FetchedCollation>
  ParachainProcessorImpl::getFromSlot(network::ParachainId id,
                                      primitives::BlockHash const &relay_parent,
                                      libp2p::peer::PeerId const &peer_id) {
    return collations_.sharedAccess(
        [&](auto const &collations)
            -> outcome::result<ParachainProcessorImpl::FetchedCollation> {
          if (auto para_it = collations.find(id); para_it != collations.end()) {
            if (auto relay_it = para_it->second.find(relay_parent);
                relay_it != para_it->second.end()) {
              if (auto peer_it = relay_it->second.find(peer_id);
                  peer_it != relay_it->second.end()) {
                return peer_it->second;
              }
            }
          }
          return Error::COLLATION_NOT_FOUND;
        });
  }

  outcome::result<ParachainProcessorImpl::FetchedCollation>
  ParachainProcessorImpl::loadIntoResponseSlot(
      network::ParachainId id,
      primitives::BlockHash const &relay_parent,
      libp2p::peer::PeerId const &peer_id,
      network::CollationFetchingResponse &&response) {
    return collations_.exclusiveAccess(
        [&](auto &collations)
            -> outcome::result<ParachainProcessorImpl::FetchedCollation> {
          auto &peer_map = collations[id][relay_parent];
          auto const &[it, inserted] = peer_map.insert(
              std::make_pair(peer_id,
                             std::make_shared<FetchedCollationState>(
                                 std::move(response),
                                 CollationState::kFetched,
                                 PoVDataState::kComplete)));

          if (!inserted) return Error::RESPONSE_ALREADY_RECEIVED;
          return it->second;
        });
  }

  void ParachainProcessorImpl::handleFetchedCollation(
      network::ParachainId id,
      primitives::BlockHash const &relay_parent,
      libp2p::peer::PeerId const &peer_id,
      network::CollationFetchingResponse &&response) {
    auto store_result =
        loadIntoResponseSlot(id, relay_parent, peer_id, std::move(response));
    if (!store_result) {
      logger_->warn("Fetch collation from {}:{} store failed with: {}",
                    peer_id.toBase58(),
                    relay_parent,
                    store_result.error().message());
      return;
    }

    FetchedCollationState const &collation = *store_result.value();
    network::CandidateDescriptor const &descriptor =
        candidateDescriptorFrom(collation);
    primitives::BlockHash const candidate_hash = candidateHashFrom(collation);

    auto const candidate_para_id = descriptor.para_id;
    if (candidate_para_id != our_current_state_.assignment) {
      logger_->warn("Try to second for para_id {} out of our assignment {}.",
                    candidate_para_id,
                    our_current_state_.assignment
                        ? std::to_string(*our_current_state_.assignment)
                        : "{no assignment}");
      return;
    }

    if (our_current_state_.seconded) {
      logger_->debug("Already have seconded block {} instead of {}.",
                     our_current_state_.seconded->toString(),
                     candidate_hash);
      return;
    }

    if (our_current_state_.issued_statements.count(candidate_hash) != 0) {
      logger_->debug("Statement of {} already issued.", candidate_hash);
      return;
    }

    appendAsyncValidationTask(id, relay_parent, peer_id);
  }

  void ParachainProcessorImpl::requestPoV() {
    logger_->error("Not implemented.");
  }

  void ParachainProcessorImpl::kickOffValidationWork(
      AttestingData &attesting_data) {
    /// TODO(iceseer): add to awaiting_validation to be sure no doublicated
    /// validations

    auto opt_descriptor = candidateDescriptorFrom(attesting_data.statement);
    BOOST_ASSERT(opt_descriptor);

    auto const &collator_id = collatorIdFromDescriptor(*opt_descriptor);
    if (our_current_state_.required_collator
        && collator_id != *our_current_state_.required_collator) {
      our_current_state_.issued_statements.insert(
          attesting_data.candidate_hash);
      return;
    }

    notify_internal(context_, [wptr{weak_from_this()}] {
      if (auto self = wptr.lock()) {
        /// request PoV
        self->requestPoV();
        /// validate PoV
      }
    });
  }

  void ParachainProcessorImpl::appendAsyncValidationTask(
      network::ParachainId id,
      primitives::BlockHash const &relay_parent,
      libp2p::peer::PeerId const &peer_id) {
    /// TODO(iceseer): add to awaiting_validation to be sure no doublicated
    /// validations
    notify_internal(
        context_, [id, relay_parent, peer_id, wptr{weak_from_this()}] {
          if (auto self = wptr.lock()) {
            self->logger_->debug(
                "Got an async task for VALIDATION execution {}:{}:{}",
                id,
                relay_parent,
                peer_id.toBase58());

            if (auto collation_result =
                    self->getFromSlot(id, relay_parent, peer_id))
              self->validateAndMakeAvailable(
                  peer_id,
                  relay_parent,
                  std::move(collation_result).value(),
                  [wptr](auto const &peer_id,
                         auto &&validate_and_second_result) {
                    if (auto self = wptr.lock())
                      self->onValidationComplete(
                          peer_id, std::move(validate_and_second_result));
                  });
            else
              self->logger_->template warn(
                  "In job retrieve collation result error: {}",
                  collation_result.error().message());
          }
        });
  }

  template <typename T>
  outcome::result<network::Signature> ParachainProcessorImpl::sign(
      T const &t) const {
    if (!keypair_) return Error::KEY_NOT_PRESENT;

    auto payload = scale::encode(t).value();
    return crypto_provider_->sign(*keypair_, payload).value();
  }

  std::optional<network::ValidatorIndex> ParachainProcessorImpl::getOurIndex() {
    logger_->error("Not implemented. Return my validator index.");
    return std::nullopt;
  }

  void ParachainProcessorImpl::handleStatement(
      libp2p::peer::PeerId const &peer_id,
      primitives::BlockHash const &relay_parent,
      std::shared_ptr<network::Statement> const &statement) {
    if (auto result = importStatement(statement)) {
      if (result->group_id != our_current_state_.assignment) {
        logger_->warn(
            "Registered statement from not our group(our: {}, registered: {}).",
            our_current_state_.assignment
                ? std::to_string(*our_current_state_.assignment)
                : "{not assigned}",
            result->group_id);
        return;
      }

      std::optional<std::reference_wrapper<AttestingData>> attesting_ref =
          visit_in_place(
              statement->candidate_state,
              [&](network::Dummy const &)
                  -> std::optional<std::reference_wrapper<AttestingData>> {
                BOOST_ASSERT(!"Not used!");
                return std::nullopt;
              },
              [&](network::CommittedCandidateReceipt const &seconded)
                  -> std::optional<std::reference_wrapper<AttestingData>> {
                auto const &candidate_hash = result->candidate;
                auto const &[it, _] =
                    our_current_state_.fallbacks.insert(std::make_pair(
                        candidate_hash,
                        AttestingData{.statement = statement,
                                      .candidate_hash = candidate_hash,
                                      .pov_hash = seconded.descriptor.pov_hash,
                                      .from_validator = statement->validator_ix,
                                      .backing = {}}));
                return it->second;
              },
              [&](primitives::BlockHash const &candidate_hash)
                  -> std::optional<std::reference_wrapper<AttestingData>> {
                auto it = our_current_state_.fallbacks.find(candidate_hash);
                if (it == our_current_state_.fallbacks.end())
                  return std::nullopt;
                if (!getOurIndex() || *getOurIndex() == statement->validator_ix)
                  return std::nullopt;
                if (our_current_state_.awaiting_validation.count(candidate_hash)
                    > 0) {
                  it->second.backing.push(statement->validator_ix);
                  return std::nullopt;
                }
                it->second.from_validator = statement->validator_ix;
                return it->second;
              });

      if (attesting_ref) kickOffValidationWork(attesting_ref->get());
    }
  }

  std::optional<ParachainProcessorImpl::ImportStatementSummary>
  ParachainProcessorImpl::importStatementToTable(
      primitives::BlockHash const &candidate_hash,
      std::shared_ptr<network::Statement> const &statement) {
    logger_->error("Not implemented. Should be inserted in statement table.");
    return std::nullopt;
  }

  void ParachainProcessorImpl::notifyBackedCandidate(
      std::shared_ptr<network::Statement> const &statement) {
    logger_->error(
        "Not implemented. Should notify somebody that backed candidate "
        "appeared.");
  }

  std::optional<ParachainProcessorImpl::ImportStatementSummary>
  ParachainProcessorImpl::importStatement(
      std::shared_ptr<network::Statement> const &statement) {
    auto import_result =
        importStatementToTable(candidateHashFrom(statement), statement);
    if (import_result && import_result->attested) {
      notifyBackedCandidate(statement);
    }
    return import_result;
  }

  template <ParachainProcessorImpl::StatementType kStatementType>
  std::shared_ptr<network::Statement>
  ParachainProcessorImpl::createAndSignStatement(
      ValidateAndSecondResult &validation_result) {
    static_assert(kStatementType == StatementType::kSeconded
                  || kStatementType == StatementType::kValid);

    auto const our_ix = getOurIndex();
    if (!our_ix) {
      logger_->template warn(
          "We are not validators or we have no validator index.");
      return nullptr;
    }

    if constexpr (kStatementType == StatementType::kSeconded) {
      network::CommittedCandidateReceipt receipt{
          .descriptor =
              candidateDescriptorFrom(*validation_result.fetched_collation),
          .commitments = std::move(*validation_result.commitments)};

      /// TODO(iceseer):
      /// https://github.com/paritytech/polkadot/blob/master/primitives/src/v2/mod.rs#L1535-L1545
      auto sign_result = sign(receipt);
      if (!sign_result) {
        logger_->error(
            "Unable to sign Commited Candidate Receipt. Failed with error: {}",
            sign_result.error().message());
        return nullptr;
      }

      return std::make_shared<network::Statement>(
          network::Statement{.candidate_state = std::move(receipt),
                             .validator_ix = *our_ix,
                             .signature = std::move(sign_result.value())});
    } else if constexpr (kStatementType == StatementType::kValid) {
      auto const candidate_hash =
          candidateHashFrom(*validation_result.fetched_collation);

      /// TODO(iceseer):
      /// https://github.com/paritytech/polkadot/blob/master/primitives/src/v2/mod.rs#L1535-L1545
      auto sign_result = sign(candidate_hash);
      if (!sign_result) {
        logger_->error(
            "Unable to sign deemed valid hash. Failed with error: {}",
            sign_result.error().message());
        return nullptr;
      }

      return std::make_shared<network::Statement>(
          network::Statement{.candidate_state = candidate_hash,
                             .validator_ix = *our_ix,
                             .signature = std::move(sign_result.value())});
    }
  }

  void ParachainProcessorImpl::handleNotify(
      libp2p::peer::PeerId const &peer_id,
      primitives::BlockHash const &relay_parent) {
    auto stream_engine = pm_->getStreamEngine();
    BOOST_ASSERT(stream_engine);

    auto collation_protocol = router_->getCollationProtocol();
    BOOST_ASSERT(collation_protocol);

    if (stream_engine->reserveOutgoing(peer_id, collation_protocol)) {
      collation_protocol->newOutgoingStream(
          libp2p::peer::PeerInfo{.id = peer_id, .addresses = {}},
          [relay_parent,
           protocol{collation_protocol},
           peer_id,
           wptr{weak_from_this()}](auto &&stream_result) {
            auto self = wptr.lock();
            if (not self) return;

            auto stream_engine = self->pm_->getStreamEngine();
            stream_engine->dropReserveOutgoing(peer_id, protocol);

            if (!stream_result.has_value()) {
              self->logger_->warn("Unable to create stream {} with {}: {}",
                                  protocol->protocolName(),
                                  peer_id,
                                  stream_result.error().message());
              return;
            }

            if (auto add_result = stream_engine->addOutgoing(
                    std::move(stream_result.value()), protocol);
                !add_result) {
              self->logger_->error("Unable to store stream {} with {}: {}",
                                   protocol->protocolName(),
                                   peer_id,
                                   add_result.error().message());
            }
            self->handleNotify(peer_id, relay_parent);
          });
      return;
    }

    auto &statements_queue = our_current_state_.seconded_statements[peer_id];
    while (!statements_queue.empty()) {
      std::shared_ptr<network::Statement> statement(statements_queue.front());
      statements_queue.pop_front();

      stream_engine->send(
          peer_id,
          collation_protocol,
          std::make_shared<network::WireMessage>(network::ProtocolMessage(
              network::CollationMessage(network::Seconded{
                  .relay_parent = relay_parent, .statement = *statement}))));
    }
  }

  void ParachainProcessorImpl::notify(
      libp2p::peer::PeerId const &peer_id,
      primitives::BlockHash const &relay_parent,
      std::shared_ptr<network::Statement> const &statement) {
    our_current_state_.seconded_statements[peer_id].emplace_back(statement);
    handleNotify(peer_id, relay_parent);
  }

  void ParachainProcessorImpl::onValidationComplete(
      libp2p::peer::PeerId const &peer_id,
      ValidateAndSecondResult &&validation_result) {
    BOOST_ASSERT(validation_result.fetched_collation);

    auto const candidate_hash =
        candidateHashFrom(*validation_result.fetched_collation);
    if (!validation_result.result) {
      logger_->warn("Candidate {} validation failed with: {}",
                    candidate_hash,
                    validation_result.result.error().message());
      // send invalid
    } else if (!our_current_state_.seconded
               && our_current_state_.issued_statements.count(candidate_hash)
                      == 0) {
      validation_result.fetched_collation->collation_state =
          CollationState::kSeconded;
      our_current_state_.seconded = candidate_hash;
      our_current_state_.issued_statements.insert(candidate_hash);
      if (auto statement = createAndSignStatement<StatementType::kSeconded>(
              validation_result)) {
        importStatement(statement);
        notifyStatementDistributionSystem(statement);
        notify(peer_id, validation_result.relay_parent, statement);
      }
    }
  }

  void ParachainProcessorImpl::notifyStatementDistributionSystem(
      std::shared_ptr<network::Statement> const &statement) {
    /// Not implemented yet
  }

  outcome::result<void> ParachainProcessorImpl::validateCandidate(
      FetchedCollation & /*fetched_collation*/) {
    logger_->error("Candidate Validation is not implemented!");
    return outcome::success();
  }

  outcome::result<void> ParachainProcessorImpl::validateErasureCoding(
      FetchedCollation & /*fetched_collation*/) {
    logger_->error("Erasure coding Validation is not implemented!");
    return outcome::success();
  }

  void ParachainProcessorImpl::notifyAvailableData() {
    logger_->error("Availability notification is not implemented!");
  }

  template <typename F>
  void ParachainProcessorImpl::validateAndMakeAvailable(
      libp2p::peer::PeerId const &peer_id,
      primitives::BlockHash const &relay_parent,
      FetchedCollation fetched_collation,
      F &&callback) {
    BOOST_ASSERT(fetched_collation);

    switch (fetched_collation->pov_state) {
      case PoVDataState::kComplete:
        break;
      case PoVDataState::kFetchFromValidator:
      default: {
        logger_->error("Unexpected PoV state processing.");
      }
        return;
    }

    if (auto validation_result = validateCandidate(fetched_collation);
        !validation_result) {
      logger_->warn("Candidate {} validation failed with error: {}",
                    candidateHashFrom(*fetched_collation),
                    validation_result.error().message());
      return;
    }

    if (auto erasure_coding_result = validateErasureCoding(fetched_collation);
        !erasure_coding_result) {
      logger_->warn("Candidate {} validation failed with error: {}",
                    candidateHashFrom(*fetched_collation),
                    erasure_coding_result.error().message());
      return;
    }

    notifyAvailableData();
    notify_internal(this_context_,
                    [peer_id,
                     callback{std::forward<F>(callback)},
                     validate_and_second_result{ValidateAndSecondResult{
                         .result = outcome::success(),
                         .fetched_collation = fetched_collation,
                         .relay_parent = relay_parent,
                         .commitments = nullptr}}]() mutable {
                      std::forward<F>(callback)(
                          peer_id, std::move(validate_and_second_result));
                    });
  }

  void ParachainProcessorImpl::onAttestComplete(
      libp2p::peer::PeerId const &peer_id, ValidateAndSecondResult &&result) {
    auto const candidate_hash = candidateHashFrom(*result.fetched_collation);
    our_current_state_.fallbacks.erase(candidate_hash);

    if (our_current_state_.issued_statements.count(candidate_hash) == 0) {
      if (result.result) {
        if (auto statement =
                createAndSignStatement<StatementType::kValid>(result)) {
          importStatement(statement);
          notifyStatementDistributionSystem(statement);
        }
      }
      our_current_state_.issued_statements.insert(candidate_hash);
    }
  }

  void ParachainProcessorImpl::onAttestNoPoVComplete(
      libp2p::peer::PeerId const &peer_id, ValidateAndSecondResult &&result) {
    auto const candidate_hash = candidateHashFrom(*result.fetched_collation);
    auto it = our_current_state_.fallbacks.find(candidate_hash);

    if (it == our_current_state_.fallbacks.end()) {
      logger_->error(
          "Internal error. Fallbacks doesn't contain candidate hash {}",
          candidate_hash);
      return;
    }

    AttestingData &attesting = it->second;
    if (!attesting.backing.empty()) {
      attesting.from_validator = attesting.backing.front();
      attesting.backing.pop();
      kickOffValidationWork(attesting);
    }
  }

  void ParachainProcessorImpl::setAssignedParachain(
      std::optional<network::ParachainId> para_id) {
    our_current_state_.assignment = para_id;
  }

  void ParachainProcessorImpl::requestCollations(
      network::PendingCollation const &pending_collation) {
    router_->getReqCollationProtocol()->request(
        pending_collation.peer_id,
        network::CollationFetchingRequest{
            .relay_parent = pending_collation.relay_parent,
            .para_id = pending_collation.para_id},
        [para_id{pending_collation.para_id},
         relay_parent{pending_collation.relay_parent},
         peer_id{pending_collation.peer_id},
         wptr{weak_from_this()}](
            outcome::result<network::CollationFetchingResponse> &&result) {
          auto self = wptr.lock();
          if (!self) return;

          if (!result) {
            self->logger_->warn("Fetch collation from {}:{} failed with: {}",
                                peer_id.toBase58(),
                                relay_parent,
                                result.error().message());
            return;
          }

          self->handleFetchedCollation(
              para_id, relay_parent, peer_id, std::move(result).value());
        });
  }

}  // namespace kagome::parachain
