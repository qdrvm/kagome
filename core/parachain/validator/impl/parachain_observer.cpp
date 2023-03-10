/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/parachain_observer.hpp"

#include <gsl/span>

#include "crypto/sr25519_provider.hpp"
#include "network/common.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/impl/protocols/protocol_error.hpp"
#include "network/peer_manager.hpp"
#include "parachain/approval/approval_distribution.hpp"
#include "parachain/validator/parachain_processor.hpp"

namespace kagome::observers {

  struct ValidationObserverImpl : network::ValidationObserver {
    ValidationObserverImpl(
        std::shared_ptr<network::PeerManager> pm,
        std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
        std::shared_ptr<parachain::ParachainProcessorImpl> processor,
        std::shared_ptr<parachain::ApprovalDistribution> approval_distribution)
        : pm_{std::move(pm)},
          crypto_provider_{std::move(crypto_provider)},
          processor_{std::move(processor)},
          approval_distribution_(std::move(approval_distribution)) {
      BOOST_ASSERT_MSG(pm_, "Peer manager must be initialized!");
      BOOST_ASSERT_MSG(processor_, "Parachain processor must be initialized!");
      BOOST_ASSERT(approval_distribution_);
    }
    ~ValidationObserverImpl() override = default;

    void onIncomingMessage(
        libp2p::peer::PeerId const &peer_id,
        network::ValidatorProtocolMessage &&message) override {
      processor_->onValidationProtocolMsg(peer_id, message);
      approval_distribution_->onValidationProtocolMsg(peer_id, message);
    }

    void onIncomingValidationStream(
        libp2p::peer::PeerId const &peer_id) override {
      processor_->onIncomingValidationStream(peer_id);
    }

   private:
    std::shared_ptr<network::PeerManager> pm_;
    std::shared_ptr<crypto::Sr25519Provider> crypto_provider_;
    std::shared_ptr<parachain::ParachainProcessorImpl> processor_;
    std::shared_ptr<parachain::ApprovalDistribution> approval_distribution_;
    log::Logger logger_ = log::createLogger("ValidationObserver", "parachain");
  };

  struct CollationObserverImpl : network::CollationObserver {
    CollationObserverImpl(
        std::shared_ptr<network::PeerManager> pm,
        std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
        std::shared_ptr<parachain::ParachainProcessorImpl> processor,
        std::shared_ptr<network::PeerView> peer_view)
        : pm_{std::move(pm)},
          crypto_provider_{std::move(crypto_provider)},
          processor_{std::move(processor)},
          peer_view_(std::move(peer_view)) {
      BOOST_ASSERT_MSG(crypto_provider_,
                       "Crypto provider must be initialized!");
      BOOST_ASSERT_MSG(pm_, "Peer manager must be initialized!");
      BOOST_ASSERT_MSG(processor_, "Parachain processor must be initialized!");
      BOOST_ASSERT(peer_view_);
    }
    ~CollationObserverImpl() override = default;

    void onIncomingCollationStream(
        libp2p::peer::PeerId const &peer_id) override {
      processor_->onIncomingCollationStream(peer_id);
    }

    void onIncomingMessage(
        libp2p::peer::PeerId const &peer_id,
        network::CollationProtocolMessage &&collation_message) override {
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

    void onAdvertise(libp2p::peer::PeerId const &peer_id,
                     primitives::BlockHash relay_parent) {
      auto const peer_state = pm_->getPeerState(peer_id);
      if (!peer_state) {
        logger_->warn("Received collation advertisement from unknown peer {}",
                      peer_id);
        return;
      }

      auto result = pm_->retrieveCollatorData(peer_state->get(), relay_parent);
      if (!result) {
        logger_->warn("Retrieve collator {} data failed: {}",
                      peer_id,
                      result.error().message());
        return;
      }

      if (auto check_res = processor_->advCanBeProcessed(relay_parent, peer_id);
          !check_res) {
        logger_->warn("Insert advertisement from {} failed: {}",
                      peer_id,
                      check_res.error().message());
        return;
      }

      logger_->info("Got advertisement from: {}, relay parent: {}",
                    peer_id,
                    relay_parent);
      processor_->requestCollations(network::CollationEvent{
          .collator_id = result.value().first,
          .pending_collation =
              network::PendingCollation{.relay_parent = relay_parent,
                                        .para_id = result.value().second,
                                        .peer_id = peer_id},
      });
    }

    void onDeclare(libp2p::peer::PeerId const &peer_id,
                   network::CollatorPublicKey pubkey,
                   network::ParachainId para_id,
                   network::Signature signature) {
      auto const peer_state = pm_->getPeerState(peer_id);
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
      logger_->info("{}:{} declared as collator with para_id {}",
                    peer_id,
                    pubkey,
                    para_id);
      processor_->onIncomingCollator(peer_id, pubkey, para_id);
    }

   private:
    std::shared_ptr<network::PeerManager> pm_;
    std::shared_ptr<crypto::Sr25519Provider> crypto_provider_;
    std::shared_ptr<parachain::ParachainProcessorImpl> processor_;
    std::shared_ptr<network::PeerView> peer_view_;
    log::Logger logger_ = log::createLogger("CollationObserver", "parachain");
  };

  struct ReqCollationObserverImpl final : network::ReqCollationObserver {
    ReqCollationObserverImpl(std::shared_ptr<network::PeerManager> pm)
        : pm_{std::move(pm)} {
      BOOST_ASSERT_MSG(pm_, "Peer manager must be initialized!");
    }
    ~ReqCollationObserverImpl() = default;

    outcome::result<network::CollationFetchingResponse> OnCollationRequest(
        network::CollationFetchingRequest request) override {
      /// Need to decrease rank of the peer and return error.
      return network::ProtocolError::PROTOCOL_NOT_IMPLEMENTED;
    }

   private:
    std::shared_ptr<network::PeerManager> pm_;
  };

  struct ReqPovObserverImpl final : network::ReqPovObserver {
    ReqPovObserverImpl(
        std::shared_ptr<parachain::ParachainProcessorImpl> processor)
        : processor_{std::move(processor)} {
      BOOST_ASSERT_MSG(processor_, "Peer manager must be initialized!");
    }
    ~ReqPovObserverImpl() = default;

    outcome::result<network::ResponsePov> OnPovRequest(
        network::RequestPov request) override {
      return processor_->getPov(std::move(request));
    }

   private:
    std::shared_ptr<parachain::ParachainProcessorImpl> processor_;
  };

}  // namespace kagome::observers

namespace kagome::parachain {

  ParachainObserverImpl::ParachainObserverImpl(
      std::shared_ptr<network::PeerManager> pm,
      std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
      std::shared_ptr<parachain::ParachainProcessorImpl> processor,
      std::shared_ptr<network::PeerView> peer_view,
      std::shared_ptr<parachain::ApprovalDistribution> approval_distribution)
      : collation_observer_impl_{std::make_shared<
          observers::CollationObserverImpl>(
          pm, std::move(crypto_provider), processor, std::move(peer_view))},
        validation_observer_impl_{
            std::make_shared<observers::ValidationObserverImpl>(
                pm,
                std::move(crypto_provider),
                processor,
                std::move(approval_distribution))},
        req_collation_observer_impl_{
            std::make_shared<observers::ReqCollationObserverImpl>(pm)},
        req_pov_observer_impl_{
            std::make_shared<observers::ReqPovObserverImpl>(processor)} {
    BOOST_ASSERT_MSG(collation_observer_impl_,
                     "Collation observer must be initialized!");
    BOOST_ASSERT_MSG(validation_observer_impl_,
                     "Validation observer must be initialized!");
    BOOST_ASSERT_MSG(req_collation_observer_impl_,
                     "Fetch collation observer must be initialized!");
    BOOST_ASSERT_MSG(req_pov_observer_impl_,
                     "Fetch pov observer must be initialized!");
  }

  void ParachainObserverImpl::onIncomingMessage(
      libp2p::peer::PeerId const &peer_id,
      network::CollationProtocolMessage &&collation_message) {
    collation_observer_impl_->onIncomingMessage(peer_id,
                                                std::move(collation_message));
  }

  void ParachainObserverImpl::onIncomingCollationStream(
      libp2p::peer::PeerId const &peer_id) {
    collation_observer_impl_->onIncomingCollationStream(peer_id);
  }

  void ParachainObserverImpl::onIncomingValidationStream(
      libp2p::peer::PeerId const &peer_id) {
    validation_observer_impl_->onIncomingValidationStream(peer_id);
  }

  void ParachainObserverImpl::onIncomingMessage(
      libp2p::peer::PeerId const &peer_id,
      network::ValidatorProtocolMessage &&validation_message) {
    validation_observer_impl_->onIncomingMessage(peer_id,
                                                 std::move(validation_message));
  }

  outcome::result<network::ResponsePov> ParachainObserverImpl::OnPovRequest(
      network::RequestPov request) {
    return req_pov_observer_impl_->OnPovRequest(std::move(request));
  }

  outcome::result<network::CollationFetchingResponse>
  ParachainObserverImpl::OnCollationRequest(
      network::CollationFetchingRequest request) {
    return req_collation_observer_impl_->OnCollationRequest(std::move(request));
  }

}  // namespace kagome::parachain
