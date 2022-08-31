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

namespace kagome::observers {

  struct CollationObserverImpl : network::CollationObserver {
    CollationObserverImpl(
        std::shared_ptr<network::PeerManager> pm,
        std::shared_ptr<crypto::Sr25519Provider> crypto_provider)
        : pm_{std::move(pm)}, crypto_provider_{std::move(crypto_provider)} {
      BOOST_ASSERT_MSG(crypto_provider_,
                       "Crypto provider must be initialised!");
      BOOST_ASSERT_MSG(pm_, "Peer manager must be initialised!");
    }
    ~CollationObserverImpl() override = default;

    void onAdvertise(libp2p::peer::PeerId const &peer_id,
                     primitives::BlockHash relay_parent) override {
      auto &parachain_state = pm_->parachainState();
      bool const contains_para_hash =
          (parachain_state.our_view.count(relay_parent) != 0);

      if (!contains_para_hash) {
        logger_->warn("Advertise collation out of view from peer {}", peer_id);
        return;
      }

      auto const peer_state = pm_->getPeerState(peer_id);
      if (!peer_state) {
        logger_->warn("Received collation advertise from unknown peer {}",
                      peer_id);
        return;
      }

      auto result = pm_->insert_advertisement(
          peer_state->get(), parachain_state, std::move(relay_parent));
      if (!result) {
        logger_->warn("Insert advertisement from {} failed: {}",
                      peer_id,
                      result.error().message());
        return;
      }

      pm_->push_pending_collation(
          network::PendingCollation{.para_id = result.value().second,
                                    .relay_parent = relay_parent,
                                    .peer_id = peer_id});
    }

    void onDeclare(libp2p::peer::PeerId const &peer_id,
                   network::CollatorPublicKey pubkey,
                   network::ParachainId para_id,
                   network::Signature signature) override {
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
      pm_->setCollating(peer_id, pubkey, para_id);
    }

   private:
    std::shared_ptr<network::PeerManager> pm_;
    std::shared_ptr<crypto::Sr25519Provider> crypto_provider_;
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
      std::shared_ptr<crypto::Sr25519Provider> crypto_provider)
      : collation_observer_impl_{std::make_shared<
          observers::CollationObserverImpl>(pm, std::move(crypto_provider))},
        req_collation_observer_impl_{
            std::make_shared<observers::ReqCollationObserverImpl>(pm)} {
    BOOST_ASSERT_MSG(collation_observer_impl_,
                     "Collation observer must be initialised!");
    BOOST_ASSERT_MSG(req_collation_observer_impl_,
                     "Fetch collation observer must be initialised!");
  }

  void ParachainObserverImpl::onAdvertise(libp2p::peer::PeerId const &peer_id,
                                          primitives::BlockHash relay_parent) {
    collation_observer_impl_->onAdvertise(peer_id, std::move(relay_parent));
  }

  void ParachainObserverImpl::onDeclare(libp2p::peer::PeerId const &peer_id,
                                        network::CollatorPublicKey pubkey,
                                        network::ParachainId para_id,
                                        network::Signature signature) {
    collation_observer_impl_->onDeclare(
        peer_id, std::move(pubkey), std::move(para_id), std::move(signature));
  }

  outcome::result<network::CollationFetchingResponse>
  ParachainObserverImpl::OnCollationRequest(
      network::CollationFetchingRequest request) {
    return req_collation_observer_impl_->OnCollationRequest(std::move(request));
  }

}  // namespace kagome::parachain
