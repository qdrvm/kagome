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
#include "parachain/validator/parachain_processor.hpp"

namespace kagome::observers {

  struct ValidationObserverImpl : network::ValidationObserver {
    ValidationObserverImpl(
        std::shared_ptr<network::PeerManager> pm,
        std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
        std::shared_ptr<parachain::ParachainProcessorImpl> processor)
        : pm_{std::move(pm)},
          crypto_provider_{std::move(crypto_provider)},
          processor_{std::move(processor)} {
      BOOST_ASSERT_MSG(pm_, "Peer manager must be initialised!");
      BOOST_ASSERT_MSG(processor_, "Parachain processor must be initialised!");
    }
    ~ValidationObserverImpl() override = default;

    void onIncomingMessage(
        libp2p::peer::PeerId const &peer_id,
        network::ValidatorProtocolMessage &&collation_message) override {}

    void onIncomingValidationStream(
        libp2p::peer::PeerId const &peer_id) override {
      processor_->onIncomingValidationStream(peer_id);
    }

   private:
    std::shared_ptr<network::PeerManager> pm_;
    std::shared_ptr<crypto::Sr25519Provider> crypto_provider_;
    std::shared_ptr<parachain::ParachainProcessorImpl> processor_;
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
                       "Crypto provider must be initialised!");
      BOOST_ASSERT_MSG(pm_, "Peer manager must be initialised!");
      BOOST_ASSERT_MSG(processor_, "Parachain processor must be initialised!");
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
      auto my_view = peer_view_->getMyView();
      BOOST_ASSERT(my_view);

      if (!my_view->get().contains(relay_parent)) {
        logger_->warn("Advertise collation out of view from peer {}", peer_id);
        return;
      }

      auto const peer_state = pm_->getPeerState(peer_id);
      if (!peer_state) {
        logger_->warn("Received collation advertise from unknown peer {}",
                      peer_id);
        return;
      }

      auto result =
          pm_->insert_advertisement(peer_state->get(), std::move(relay_parent));
      if (!result) {
        logger_->warn("Insert advertisement from {} failed: {}",
                      peer_id,
                      result.error().message());
        return;
      }

      processor_->requestCollations(
          network::PendingCollation{.para_id = result.value().second,
                                    .relay_parent = relay_parent,
                                    .peer_id = peer_id});
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
        return;
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
      processor_->onIncomingCollator(peer_id, pubkey, para_id);
    }

   private:
    std::shared_ptr<network::PeerManager> pm_;
    std::shared_ptr<crypto::Sr25519Provider> crypto_provider_;
    std::shared_ptr<parachain::ParachainProcessorImpl> processor_;
    std::shared_ptr<network::PeerView> peer_view_;
    log::Logger logger_ = log::createLogger("CollationObserver", "parachain");
  };

  struct ReqCollationObserverImpl : network::ReqCollationObserver {
    ReqCollationObserverImpl(std::shared_ptr<network::PeerManager> pm)
        : pm_{std::move(pm)} {
      BOOST_ASSERT_MSG(pm_, "Peer manager must be initialised!");
    }
    ~ReqCollationObserverImpl() override = default;

    outcome::result<network::CollationFetchingResponse> OnCollationRequest(
        network::CollationFetchingRequest request) override {
      /// Need to decrease rank of the peer and return error.
      return network::ProtocolError::PROTOCOL_NOT_IMPLEMENTED;
    }

   private:
    std::shared_ptr<network::PeerManager> pm_;
  };

}  // namespace kagome::observers

namespace kagome::parachain {

  ParachainObserverImpl::ParachainObserverImpl(
      std::shared_ptr<network::PeerManager> pm,
      std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
      std::shared_ptr<parachain::ParachainProcessorImpl> processor,
      std::shared_ptr<network::PeerView> peer_view)
      : collation_observer_impl_{std::make_shared<
          observers::CollationObserverImpl>(
          pm, std::move(crypto_provider), processor, std::move(peer_view))},
        validation_observer_impl_{
            std::make_shared<observers::ValidationObserverImpl>(
                pm, std::move(crypto_provider), processor)},
        req_collation_observer_impl_{
            std::make_shared<observers::ReqCollationObserverImpl>(pm)},
        processor_{processor} {
    BOOST_ASSERT_MSG(collation_observer_impl_,
                     "Collation observer must be initialised!");
    BOOST_ASSERT_MSG(validation_observer_impl_,
                     "Validation observer must be initialised!");
    BOOST_ASSERT_MSG(req_collation_observer_impl_,
                     "Fetch collation observer must be initialised!");
    BOOST_ASSERT_MSG(processor_, "Parachain processor must be initialised!");
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

  outcome::result<network::CollationFetchingResponse>
  ParachainObserverImpl::OnCollationRequest(
      network::CollationFetchingRequest request) {
    return req_collation_observer_impl_->OnCollationRequest(std::move(request));
  }

}  // namespace kagome::parachain
