/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/parachain_processor.hpp"

#include <array>
#include <gsl/span>
#include <unordered_map>

#include "crypto/hasher.hpp"
#include "crypto/sr25519_provider.hpp"
#include "network/common.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/impl/protocols/protocol_error.hpp"
#include "network/peer_manager.hpp"
#include "network/router.hpp"
#include "parachain/candidate_view.hpp"
#include "parachain/peer_relay_parent_knowledge.hpp"
#include "scale/scale.hpp"
#include "utils/async_sequence.hpp"

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
    case E::VALIDATION_FAILED:
      return "Validate and make available failed";
    case E::VALIDATION_SKIPPED:
      return "Validate and make available skipped";
  }
  return "Unknown parachain processor error";
}

#if __cplusplus < 202002L
namespace std {
  template <typename Container, typename Pred>
  inline auto erase_if(Container &c, Pred &&pred) ->
      typename Container::size_type {
    const auto old_size = c.size();
    for (auto it = c.begin(); it != c.end();) {
      if (std::forward<Pred>(pred)(*it)) {
        it = c.erase(it);
      } else {
        ++it;
      }
    }
    return old_size - c.size();
  }
}  // namespace std
#endif

namespace kagome::parachain {

  ParachainProcessorImpl::ParachainProcessorImpl(
      std::shared_ptr<network::PeerManager> pm,
      std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
      std::shared_ptr<network::Router> router,
      std::shared_ptr<boost::asio::io_context> this_context,
      std::shared_ptr<crypto::Sr25519Keypair> keypair,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<network::PeerView> peer_view,
      std::shared_ptr<ThreadPool> thread_pool)
      : pm_(std::move(pm)),
        crypto_provider_(std::move(crypto_provider)),
        router_(std::move(router)),
        this_context_(std::move(this_context)),
        keypair_(std::move(keypair)),
        hasher_(std::move(hasher)),
        peer_view_(std::move(peer_view)),
        thread_pool_(std::move(thread_pool)) {
    BOOST_ASSERT(pm_);
    BOOST_ASSERT(peer_view_);
    BOOST_ASSERT(crypto_provider_);
    BOOST_ASSERT(this_context_);
    BOOST_ASSERT(router_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(thread_pool_);
  }

  bool ParachainProcessorImpl::prepare() {
    my_view_sub_ = std::make_shared<network::PeerView::MyViewSubscriber>(
        peer_view_->getMyViewObservable(), false);
    my_view_sub_->subscribe(my_view_sub_->generateSubscriptionSetId(),
                            network::PeerView::EventType::kViewUpdated);
    my_view_sub_->setCallback([wptr{weak_from_this()}](
                                  auto /*set_id*/,
                                  auto && /*internal_obj*/,
                                  auto /*event_type*/,
                                  const network::View &event) {
      if (auto self = wptr.lock()) {
        auto msg = std::make_shared<
            network::WireMessage<network::ValidatorProtocolMessage>>(
            network::ViewUpdate{.view = event});

        self->pm_->getStreamEngine()->broadcast(
            self->router_->getValidationProtocol(), msg);
        self->pm_->getStreamEngine()->broadcast(
            self->router_->getCollationProtocol(), msg);

        /// clear caches
        std::erase_if(
            self->our_current_state_.state_by_relay_parent,
            [&](auto const &it) { return !event.contains(it.first); });
        self->collations_.exclusiveAccess([&](auto &collations) {
          std::erase_if(collations, [&](auto &it_para) {
            std::erase_if(it_para.second,
                          [&](auto &it) { return !event.contains(it.first); });
            return it_para.second.empty();
          });
        });
      }
    });
    return true;
  }

  bool ParachainProcessorImpl::start() {
    return true;
  }

  void ParachainProcessorImpl::stop() {}

  outcome::result<ParachainProcessorImpl::FetchedCollation>
  ParachainProcessorImpl::getFromSlot(network::ParachainId id,
                                      primitives::BlockHash const &relay_parent,
                                      libp2p::peer::PeerId const &peer_id) {
    auto fetched_collation = collations_.sharedAccess(
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

    if (fetched_collation.has_error()) {
      logger_->warn("getFromSlot fetched collation failed with error: {}",
                    fetched_collation.error().message());
    }

    return fetched_collation;
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
                    store_result.error());
      return;
    }

    auto &parachain_state = getStateByRelayParent(relay_parent);
    auto &assignment = parachain_state.assignment;
    auto &seconded = parachain_state.seconded;
    auto &issued_statements = parachain_state.issued_statements;

    FetchedCollationState const &collation = *store_result.value();
    network::CandidateDescriptor const &descriptor =
        candidateDescriptorFrom(collation);
    primitives::BlockHash const candidate_hash = candidateHashFrom(collation);

    auto const candidate_para_id = descriptor.para_id;
    if (candidate_para_id != assignment) {
      logger_->warn(
          "Try to second for para_id {} out of our assignment {}.",
          candidate_para_id,
          assignment ? std::to_string(*assignment) : "{no assignment}");
      return;
    }

    if (seconded) {
      logger_->debug("Already have seconded block {} instead of {}.",
                     seconded->toString(),
                     candidate_hash);
      return;
    }

    if (issued_statements.count(candidate_hash) != 0) {
      logger_->debug("Statement of {} already issued.", candidate_hash);
      return;
    }

    appendAsyncValidationTask(
        id, relay_parent, peer_id, parachain_state, candidate_hash);
  }

  void ParachainProcessorImpl::requestPoV() {
    logger_->error("Not implemented.");
  }

  void ParachainProcessorImpl::kickOffValidationWork(
      AttestingData &attesting_data, RelayParentState &parachain_state) {
    parachain_state.awaiting_validation.insert(attesting_data.candidate_hash);
    auto opt_descriptor = candidateDescriptorFrom(
        parachain::getPayload(*attesting_data.statement));
    BOOST_ASSERT(opt_descriptor);

    auto const &collator_id = collatorIdFromDescriptor(*opt_descriptor);
    if (our_current_state_.required_collator
        && collator_id != *our_current_state_.required_collator) {
      parachain_state.issued_statements.insert(attesting_data.candidate_hash);
      return;
    }

    sequenceIgnore(
        thread_pool_->io_context()->wrap(asAsync([wptr{weak_from_this()}]() mutable -> outcome::result<void> {
          if (auto self = wptr.lock()) {
            /// request PoV
            self->requestPoV();
            /// validate PoV
          }
          return outcome::success();
        })));
  }

  void ParachainProcessorImpl::appendAsyncValidationTask(
      network::ParachainId id,
      primitives::BlockHash const &relay_parent,
      libp2p::peer::PeerId const &peer_id,
      RelayParentState &parachain_state,
      const primitives::BlockHash &candidate_hash) {
    parachain_state.awaiting_validation.insert(candidate_hash);
    sequenceIgnore(
        thread_pool_->io_context()->wrap(
            asAsync([id, relay_parent, peer_id, wptr{weak_from_this()}]() mutable
                        -> outcome::result<ValidateAndSecondResult> {
              auto self = wptr.lock();
              if (!self) {
                return Error::VALIDATION_SKIPPED;
              }
              self->logger_->debug(
                  "Got an async task for VALIDATION execution {}:{}:{}",
                  id,
                  relay_parent,
                  peer_id.toBase58());

              OUTCOME_TRY(collation_result,
                          self->getFromSlot(id, relay_parent, peer_id));
              return self->validateAndMakeAvailable(
                  wptr, peer_id, relay_parent, std::move(collation_result));
            })),
        this_context_->wrap(
            asAsync([peer_id, wptr{weak_from_this()}](
                        auto &&validate_and_second_result) mutable -> outcome::result<void> {
              if (auto self = wptr.lock()) {
                auto parachain_state = self->tryGetStateByRelayParent(
                    validate_and_second_result.relay_parent);

                BOOST_ASSERT(parachain_state);
                parachain_state->get().awaiting_validation.erase(
                    self->candidateHashFrom(
                        *validate_and_second_result.fetched_collation));
                auto q{std::move(validate_and_second_result)};
                self->onValidationComplete(
                    peer_id, std::move(q));
              }
              return outcome::success();
            })));
  }

  template <typename T>
  outcome::result<network::Signature> ParachainProcessorImpl::sign(
      T const &t) const {
    if (!keypair_) return Error::KEY_NOT_PRESENT;

    auto payload = scale::encode(t).value();
    return crypto_provider_->sign(*keypair_, payload).value();
  }

  std::optional<
      std::reference_wrapper<ParachainProcessorImpl::RelayParentState>>
  ParachainProcessorImpl::tryGetStateByRelayParent(
      const primitives::BlockHash &relay_parent) {
    auto const it = our_current_state_.state_by_relay_parent.find(relay_parent);
    if (it != our_current_state_.state_by_relay_parent.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  ParachainProcessorImpl::RelayParentState &
  ParachainProcessorImpl::getStateByRelayParent(
      const primitives::BlockHash &relay_parent) {
    return our_current_state_.state_by_relay_parent[relay_parent];
  }

  void ParachainProcessorImpl::handleStatement(
      libp2p::peer::PeerId const &peer_id,
      primitives::BlockHash const &relay_parent,
      std::shared_ptr<network::SignedStatement> const &statement) {
    auto &parachain_state = getStateByRelayParent(relay_parent);
    auto &assignment = parachain_state.assignment;
    auto &fallbacks = parachain_state.fallbacks;
    auto &awaiting_validation = parachain_state.awaiting_validation;

    if (auto result = importStatement(statement)) {
      if (result->group_id != assignment) {
        logger_->warn(
            "Registered statement from not our group(our: {}, registered: {}).",
            assignment ? std::to_string(*assignment) : "{not assigned}",
            result->group_id);
        return;
      }

      std::optional<std::reference_wrapper<AttestingData>> attesting_ref =
          visit_in_place(
              parachain::getPayload(*statement).candidate_state,
              [&](network::Dummy const &)
                  -> std::optional<std::reference_wrapper<AttestingData>> {
                BOOST_ASSERT(!"Not used!");
                return std::nullopt;
              },
              [&](network::CommittedCandidateReceipt const &seconded)
                  -> std::optional<std::reference_wrapper<AttestingData>> {
                auto const &candidate_hash = result->candidate;
                auto const &[it, _] = fallbacks.insert(std::make_pair(
                    candidate_hash,
                    AttestingData{.statement = statement,
                                  .candidate_hash = candidate_hash,
                                  .pov_hash = seconded.descriptor.pov_hash,
                                  .from_validator = statement->payload.ix,
                                  .backing = {}}));
                return it->second;
              },
              [&](primitives::BlockHash const &candidate_hash)
                  -> std::optional<std::reference_wrapper<AttestingData>> {
                auto it = fallbacks.find(candidate_hash);
                if (it == fallbacks.end()) return std::nullopt;
                if (!parachain_state.our_index
                    || *parachain_state.our_index == statement->payload.ix)
                  return std::nullopt;
                if (awaiting_validation.count(candidate_hash) > 0) {
                  it->second.backing.push(statement->payload.ix);
                  return std::nullopt;
                }
                it->second.from_validator = statement->payload.ix;
                return it->second;
              });

      if (attesting_ref)
        kickOffValidationWork(attesting_ref->get(), parachain_state);
    }
  }

  std::optional<ParachainProcessorImpl::ImportStatementSummary>
  ParachainProcessorImpl::importStatementToTable(
      primitives::BlockHash const &candidate_hash,
      std::shared_ptr<network::SignedStatement> const &statement) {
    logger_->error("Not implemented. Should be inserted in statement table.");
    return std::nullopt;
  }

  void ParachainProcessorImpl::notifyBackedCandidate(
      std::shared_ptr<network::SignedStatement> const &statement) {
    logger_->error(
        "Not implemented. Should notify somebody that backed candidate "
        "appeared.");
  }

  std::optional<ParachainProcessorImpl::ImportStatementSummary>
  ParachainProcessorImpl::importStatement(
      std::shared_ptr<network::SignedStatement> const &statement) {
    auto import_result = importStatementToTable(
        candidateHashFrom(parachain::getPayload(*statement)), statement);
    if (import_result && import_result->attested) {
      notifyBackedCandidate(statement);
    }
    return import_result;
  }

  template <ParachainProcessorImpl::StatementType kStatementType>
  std::shared_ptr<network::SignedStatement>
  ParachainProcessorImpl::createAndSignStatement(
      ValidateAndSecondResult &validation_result) {
    static_assert(kStatementType == StatementType::kSeconded
                  || kStatementType == StatementType::kValid);

    auto parachain_state =
        tryGetStateByRelayParent(validation_result.relay_parent);
    if (!parachain_state) {
      logger_->error("Create and sign statement. No such relay_parent {}.",
                     validation_result.relay_parent);
      return nullptr;
    }

    if (!parachain_state->get().our_index) {
      logger_->template warn(
          "We are not validators or we have no validator index.");
      return nullptr;
    }

    if constexpr (kStatementType == StatementType::kSeconded) {
      return createAndSignStatementFromPayload(
          network::CommittedCandidateReceipt{
              .descriptor =
                  candidateDescriptorFrom(*validation_result.fetched_collation),
              .commitments = std::move(*validation_result.commitments)},
          *parachain_state->get().our_index);
    } else if constexpr (kStatementType == StatementType::kValid) {
      return createAndSignStatementFromPayload(
          candidateHashFrom(*validation_result.fetched_collation),
          *parachain_state->get().our_index);
    }
  }

  template <typename T>
  std::shared_ptr<network::SignedStatement>
  ParachainProcessorImpl::createAndSignStatementFromPayload(
      T &&payload, network::ValidatorIndex our_ix) {
    /// TODO(iceseer):
    /// https://github.com/paritytech/polkadot/blob/master/primitives/src/v2/mod.rs#L1535-L1545
    auto sign_result = sign(payload);
    if (!sign_result) {
      logger_->error(
          "Unable to sign Commited Candidate Receipt. Failed with error: {}",
          sign_result.error().message());
      return nullptr;
    }

    return std::make_shared<network::SignedStatement>(network::SignedStatement{
        .payload = {.payload = network::Statement{.candidate_state =
                                                      std::forward<T>(payload)},
                    .ix = our_ix},
        .signature = std::move(sign_result.value())});
  }

  template <typename F>
  bool ParachainProcessorImpl::tryOpenOutgoingStream(
      libp2p::peer::PeerId const &peer_id,
      std::shared_ptr<network::ProtocolBase> protocol,
      F &&callback) {
    auto stream_engine = pm_->getStreamEngine();
    BOOST_ASSERT(stream_engine);

    if (stream_engine->reserveOutgoing(peer_id, protocol)) {
      protocol->newOutgoingStream(
          libp2p::peer::PeerInfo{.id = peer_id, .addresses = {}},
          [callback{std::forward<F>(callback)},
           protocol,
           peer_id,
           wptr{weak_from_this()}](auto &&stream_result) mutable {
            auto self = wptr.lock();
            if (not self) return;

            auto stream_engine = self->pm_->getStreamEngine();
            stream_engine->dropReserveOutgoing(peer_id, protocol);

            if (!stream_result.has_value()) {
              self->logger_->warn("Unable to create stream {} with {}: {}",
                                  protocol->protocolName(),
                                  peer_id,
                                  stream_result.error());
              return;
            }

            auto stream = stream_result.value();
            if (auto add_result = stream_engine->addOutgoing(
                    std::move(stream_result.value()), protocol);
                !add_result) {
              self->logger_->error("Unable to store stream {} with {}: {}",
                                   protocol->protocolName(),
                                   peer_id,
                                   add_result.error().message());
              return;
            }

            std::forward<F>(callback)(std::move(stream));
          });
      return true;
    }
    return false;
  }

  template <typename F>
  bool ParachainProcessorImpl::tryOpenOutgoingCollatingStream(
      libp2p::peer::PeerId const &peer_id, F &&callback) {
    auto protocol = router_->getCollationProtocol();
    BOOST_ASSERT(protocol);

    return tryOpenOutgoingStream(
        peer_id, std::move(protocol), std::forward<F>(callback));
  }

  template <typename F>
  bool ParachainProcessorImpl::tryOpenOutgoingValidationStream(
      libp2p::peer::PeerId const &peer_id, F &&callback) {
    auto protocol = router_->getValidationProtocol();
    BOOST_ASSERT(protocol);

    return tryOpenOutgoingStream(
        peer_id, std::move(protocol), std::forward<F>(callback));
  }

  void ParachainProcessorImpl::sendMyView(
      const libp2p::peer::PeerId &peer_id,
      const std::shared_ptr<network::Stream> &stream,
      const std::shared_ptr<network::ProtocolBase> &protocol) {
    auto my_view = peer_view_->getMyView();
    BOOST_ASSERT(my_view);
    BOOST_ASSERT(protocol);

    pm_->getStreamEngine()->template send(
        peer_id,
        protocol,
        std::make_shared<
            network::WireMessage<network::ValidatorProtocolMessage>>(
            network::ViewUpdate{.view = my_view->get()}));
  }

  void ParachainProcessorImpl::onIncomingCollationStream(
      libp2p::peer::PeerId const &peer_id) {
    if (tryOpenOutgoingCollatingStream(
            peer_id, [wptr{weak_from_this()}, peer_id](auto &&stream) {
              if (auto self = wptr.lock()) {
                self->sendMyView(
                    peer_id, stream, self->router_->getCollationProtocol());
              }
            })) {
      logger_->info("Initiated collation protocol with {}", peer_id);
    }
  }

  void ParachainProcessorImpl::onIncomingValidationStream(
      libp2p::peer::PeerId const &peer_id) {
    if (tryOpenOutgoingValidationStream(
            peer_id, [wptr{weak_from_this()}, peer_id](auto &&stream) {
              if (auto self = wptr.lock()) {
                self->sendMyView(
                    peer_id, stream, self->router_->getValidationProtocol());
              }
            })) {
      logger_->info("Initiated validation protocol with {}", peer_id);
    }
  }

  void ParachainProcessorImpl::onIncomingCollator(
      libp2p::peer::PeerId const &peer_id,
      network::CollatorPublicKey pubkey,
      network::ParachainId para_id) {
    pm_->setCollating(peer_id, pubkey, para_id);
  }

  void ParachainProcessorImpl::handleNotify(
      libp2p::peer::PeerId const &peer_id,
      primitives::BlockHash const &relay_parent) {
    if (tryOpenOutgoingCollatingStream(
            peer_id,
            [peer_id, relay_parent, wptr{weak_from_this()}](
                auto && /*stream*/) {
              auto self = wptr.lock();
              if (not self) {
                return;
              }
              self->handleNotify(peer_id, relay_parent);
            })) {
      return;
    }

    auto stream_engine = pm_->getStreamEngine();
    BOOST_ASSERT(stream_engine);

    auto collation_protocol = router_->getCollationProtocol();
    BOOST_ASSERT(collation_protocol);

    auto &statements_queue = our_current_state_.seconded_statements[peer_id];
    while (!statements_queue.empty()) {
      std::shared_ptr<network::SignedStatement> statement(
          statements_queue.front());
      statements_queue.pop_front();

      stream_engine->send(
          peer_id,
          collation_protocol,
          std::make_shared<
              network::WireMessage<network::CollationProtocolMessage>>(
              network::CollationProtocolMessage(network::CollationMessage(
                  network::Seconded{.relay_parent = relay_parent,
                                    .statement = *statement}))));
    }
  }

  void ParachainProcessorImpl::notify(
      libp2p::peer::PeerId const &peer_id,
      primitives::BlockHash const &relay_parent,
      std::shared_ptr<network::SignedStatement> const &statement) {
    our_current_state_.seconded_statements[peer_id].emplace_back(statement);
    handleNotify(peer_id, relay_parent);
  }

  void ParachainProcessorImpl::onValidationComplete(
      libp2p::peer::PeerId const &peer_id,
      ValidateAndSecondResult &&validation_result) {
    BOOST_ASSERT(validation_result.fetched_collation);

    auto &parachain_state =
        getStateByRelayParent(validation_result.relay_parent);
    auto &seconded = parachain_state.seconded;
    auto const candidate_hash =
        candidateHashFrom(*validation_result.fetched_collation);
    if (!validation_result.result) {
      logger_->warn("Candidate {} validation failed with: {}",
                    candidate_hash,
                    validation_result.result.error());
      // send invalid
    } else if (!seconded
               && parachain_state.issued_statements.count(candidate_hash)
                      == 0) {
      validation_result.fetched_collation->collation_state =
          CollationState::kSeconded;
      seconded = candidate_hash;
      parachain_state.issued_statements.insert(candidate_hash);
      if (auto statement = createAndSignStatement<StatementType::kSeconded>(
              validation_result)) {
        importStatement(statement);
        notifyStatementDistributionSystem(validation_result.relay_parent,
                                          statement);
        notify(peer_id, validation_result.relay_parent, statement);
      }
    }
  }

  void ParachainProcessorImpl::notifyStatementDistributionSystem(
      primitives::BlockHash const &relay_parent,
      std::shared_ptr<network::SignedStatement> const &statement) {
    BOOST_ASSERT(statement);
    auto se = pm_->getStreamEngine();
    BOOST_ASSERT(se);

    se->broadcast(
        router_->getValidationProtocol(),
        std::make_shared<
            network::WireMessage<network::ValidatorProtocolMessage>>(
            network::ValidatorProtocolMessage{
                network::StatementDistributionMessage{network::Seconded{
                    .relay_parent = relay_parent, .statement = *statement}}}));
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

  outcome::result<ParachainProcessorImpl::ValidateAndSecondResult>
  ParachainProcessorImpl::validateAndMakeAvailable(
      std::weak_ptr<ParachainProcessorImpl> wptr,
      libp2p::peer::PeerId const &peer_id,
      primitives::BlockHash const &relay_parent,
      FetchedCollation fetched_collation) {
    BOOST_ASSERT(fetched_collation);

    switch (fetched_collation->pov_state) {
      case PoVDataState::kComplete:
        break;
      case PoVDataState::kFetchFromValidator:
      default: {
        logger_->error("Unexpected PoV state processing.");
      }
        return Error::VALIDATION_FAILED;
    }

    if (auto validation_result = validateCandidate(fetched_collation);
        !validation_result) {
      logger_->warn("Candidate {} validation failed with error: {}",
                    candidateHashFrom(*fetched_collation),
                    validation_result.error().message());
      return Error::VALIDATION_FAILED;
    }

    if (auto erasure_coding_result = validateErasureCoding(fetched_collation);
        !erasure_coding_result) {
      logger_->warn("Candidate {} validation failed with error: {}",
                    candidateHashFrom(*fetched_collation),
                    erasure_coding_result.error().message());
      return Error::VALIDATION_FAILED;
    }

    notifyAvailableData();
    return ValidateAndSecondResult{.result = outcome::success(),
                                   .fetched_collation = fetched_collation,
                                   .relay_parent = relay_parent,
                                   .commitments = nullptr};
  }

  void ParachainProcessorImpl::onAttestComplete(
      libp2p::peer::PeerId const &peer_id, ValidateAndSecondResult &&result) {
    auto parachain_state = tryGetStateByRelayParent(result.relay_parent);
    if (!parachain_state) {
      logger_->warn(
          "onAttestComplete result based on unexpected relay_parent {}",
          result.relay_parent);
      return;
    }

    auto const candidate_hash = candidateHashFrom(*result.fetched_collation);
    parachain_state->get().fallbacks.erase(candidate_hash);

    if (parachain_state->get().issued_statements.count(candidate_hash) == 0) {
      if (result.result) {
        if (auto statement =
                createAndSignStatement<StatementType::kValid>(result)) {
          importStatement(statement);
          notifyStatementDistributionSystem(result.relay_parent, statement);
        }
      }
      parachain_state->get().issued_statements.insert(candidate_hash);
    }
  }

  void ParachainProcessorImpl::onAttestNoPoVComplete(
      libp2p::peer::PeerId const &peer_id, ValidateAndSecondResult &&result) {
    auto parachain_state = tryGetStateByRelayParent(result.relay_parent);
    if (!parachain_state) {
      logger_->warn(
          "onAttestNoPoVComplete result based on unexpected relay_parent {}",
          result.relay_parent);
      return;
    }

    auto const candidate_hash = candidateHashFrom(*result.fetched_collation);
    auto it = parachain_state->get().fallbacks.find(candidate_hash);

    if (it == parachain_state->get().fallbacks.end()) {
      logger_->error(
          "Internal error. Fallbacks doesn't contain candidate hash {}",
          candidate_hash);
      return;
    }

    AttestingData &attesting = it->second;
    if (!attesting.backing.empty()) {
      attesting.from_validator = attesting.backing.front();
      attesting.backing.pop();
      kickOffValidationWork(attesting, *parachain_state);
    }
  }

  void ParachainProcessorImpl::setAssignedParachain(
      std::optional<network::ParachainId> para_id,
      const primitives::BlockHash &relay_parent) {
    auto &parachain_state = getStateByRelayParent(relay_parent);
    parachain_state.assignment = para_id;
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
                                result.error());
            return;
          }

          self->handleFetchedCollation(
              para_id, relay_parent, peer_id, std::move(result).value());
        });
  }

}  // namespace kagome::parachain
