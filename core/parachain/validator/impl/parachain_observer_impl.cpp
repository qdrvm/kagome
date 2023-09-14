/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/impl/parachain_observer_impl.hpp"

#include <gsl/span>

#include "crypto/sr25519_provider.hpp"
#include "network/common.hpp"
#include "network/impl/protocols/protocol_error.hpp"
#include "network/peer_manager.hpp"
#include "parachain/approval/approval_distribution.hpp"
#include "parachain/validator/parachain_processor.hpp"

namespace kagome::parachain {

  ParachainObserverImpl::ParachainObserverImpl(
      std::shared_ptr<network::PeerManager> pm,
      std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
      std::shared_ptr<parachain::ParachainProcessorImpl> processor,
      std::shared_ptr<network::PeerView> peer_view,
      std::shared_ptr<parachain::ApprovalDistribution> approval_distribution)
      : pm_{std::move(pm)},
        crypto_provider_{std::move(crypto_provider)},
        processor_{std::move(processor)},
        peer_view_(std::move(peer_view)),
        approval_distribution_(std::move(approval_distribution)),
        logger_(log::createLogger("ParachainObserver", "parachain")) {
    BOOST_ASSERT_MSG(pm_, "Peer manager must be initialized!");
    BOOST_ASSERT_MSG(crypto_provider_, "Crypto provider must be initialized!");
    BOOST_ASSERT_MSG(processor_, "Parachain processor must be initialized!");
    BOOST_ASSERT(peer_view_);
    BOOST_ASSERT(approval_distribution_);
  }

  void ParachainObserverImpl::onIncomingMessage(
      const libp2p::peer::PeerId &peer_id,
      network::CollationProtocolMessage &&collation_message) {
    visit_in_place(
        std::move(collation_message),
        [&](network::CollationMessage &&collation_msg) {
          visit_in_place(
              std::move(collation_msg),
              [&](network::CollatorDeclaration &&collation_decl) {
                onDeclare(peer_id,
                          std::move(collation_decl.collator_id),
                          std::move(collation_decl.para_id),
                          std::move(collation_decl.signature));
              },
              [&](network::CollatorAdvertisement &&collation_adv) {
                onAdvertise(peer_id, std::move(collation_adv.relay_parent));
              },
              [&](auto &&) {
                SL_WARN(logger_, "Unexpected collation message from.");
              });
        },
        [&](auto &&) {
          SL_WARN(logger_, "Unexpected collation message from.");
        });
  }

  void ParachainObserverImpl::onIncomingCollationStream(
      const libp2p::peer::PeerId &peer_id) {
    processor_->onIncomingCollationStream(peer_id);
  }

  void ParachainObserverImpl::onIncomingValidationStream(
      const libp2p::peer::PeerId &peer_id) {
    processor_->onIncomingValidationStream(peer_id);
  }

  void ParachainObserverImpl::onIncomingMessage(
      const libp2p::peer::PeerId &peer_id,
      network::ValidatorProtocolMessage &&message) {
    processor_->onValidationProtocolMsg(peer_id, message);
    approval_distribution_->onValidationProtocolMsg(peer_id, message);
  }

  outcome::result<network::ResponsePov> ParachainObserverImpl::OnPovRequest(
      network::RequestPov request) {
    return processor_->getPov(std::move(request));
  }

  outcome::result<network::CollationFetchingResponse>
  ParachainObserverImpl::OnCollationRequest(
      network::CollationFetchingRequest request) {
    /// Need to decrease rank of the peer and return error.
    return network::ProtocolError::PROTOCOL_NOT_IMPLEMENTED;
  }

  void ParachainObserverImpl::onAdvertise(const libp2p::peer::PeerId &peer_id,
                                          primitives::BlockHash relay_parent) {
    const auto peer_state = pm_->getPeerState(peer_id);
    if (!peer_state) {
      logger_->warn("Received collation advertisement from unknown peer {}",
                    peer_id);
      return;
    }

    auto result = pm_->retrieveCollatorData(peer_state->get(), relay_parent);
    if (!result) {
      logger_->warn(
          "Retrieve collator {} data failed: {}", peer_id, result.error());
      return;
    }

    if (auto check_res = processor_->advCanBeProcessed(relay_parent, peer_id);
        !check_res) {
      logger_->warn("Insert advertisement from {} failed: {}",
                    peer_id,
                    check_res.error().message());
      return;
    }

    logger_->info(
        "Got advertisement from: {}, relay parent: {}", peer_id, relay_parent);
    processor_->requestCollations(network::CollationEvent{
        .collator_id = result.value().first,
        .pending_collation =
            network::PendingCollation{.relay_parent = relay_parent,
                                      .para_id = result.value().second,
                                      .peer_id = peer_id},
    });
  }

  void ParachainObserverImpl::onDeclare(const libp2p::peer::PeerId &peer_id,
                                        network::CollatorPublicKey pubkey,
                                        network::ParachainId para_id,
                                        network::Signature signature) {
    const auto peer_state = pm_->getPeerState(peer_id);
    if (!peer_state) {
      logger_->warn("Received collation declaration from unknown peer {}:{}",
                    peer_id,
                    para_id);
      return;
    }

    if (peer_state->get().collator_state) {
      logger_->warn("Peer is in collating state {}:{}", peer_id, para_id);
      // TODO(iceseer): https://github.com/soramitsu/kagome/issues/1513 check
      // that peer is not in collating state, or is in collating state with
      // similar pubkey and parachain id.
    }

    auto payload{peer_id.toVector()};  /// Copy because verify works with
                                       /// non-constant value.
    payload.insert(payload.end(), {'C', 'O', 'L', 'L'});

    if (auto result = crypto_provider_->verify(
            signature, gsl::span<uint8_t>(payload), pubkey);
        !result) {
      logger_->warn("Received incorrect collation declaration from {}:{}",
                    peer_id,
                    para_id);
      return;
    }

    /// need to set active paras based on ViewChanged events.
    logger_->info(
        "{}:{} declared as collator with para_id {}", peer_id, pubkey, para_id);
    processor_->onIncomingCollator(peer_id, pubkey, para_id);
  }

}  // namespace kagome::parachain
