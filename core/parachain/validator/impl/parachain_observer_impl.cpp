/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/impl/parachain_observer_impl.hpp"

#include <span>

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
      std::shared_ptr<parachain::ParachainProcessor> processor,
      std::shared_ptr<parachain::ParachainStorage> parachain_storage,
      std::shared_ptr<network::PeerView> peer_view,
      std::shared_ptr<parachain::ApprovalDistribution> approval_distribution)
      : pm_{std::move(pm)},
        crypto_provider_{std::move(crypto_provider)},
        processor_{std::move(processor)},
        parachain_storage_(std::move(parachain_storage)),
        peer_view_(std::move(peer_view)),
        approval_distribution_(std::move(approval_distribution)),
        logger_(log::createLogger("ParachainObserver", "parachain")) {
    BOOST_ASSERT_MSG(pm_, "Peer manager must be initialized!");
    BOOST_ASSERT_MSG(crypto_provider_, "Crypto provider must be initialized!");
    BOOST_ASSERT_MSG(processor_, "Parachain processor must be initialized!");
    BOOST_ASSERT(peer_view_);
    BOOST_ASSERT(approval_distribution_);
    BOOST_ASSERT(parachain_storage_);
  }

  void ParachainObserverImpl::onIncomingMessage(
      const libp2p::peer::PeerId &peer_id,
      network::VersionedCollatorProtocolMessage &&msg) {
    visit_in_place(
        std::move(msg),
        [&](kagome::network::CollationMessage0 &&msg0) {
          visit_in_place(
              std::move(boost::get<network::CollationMessage>(msg0)),
              [&](network::CollatorDeclaration &&collation_decl) {
                onDeclare(peer_id,
                          collation_decl.collator_id,
                          collation_decl.para_id,
                          collation_decl.signature);
              },
              [&](network::CollatorAdvertisement &&collation_adv) {
                onAdvertise(peer_id,
                            collation_adv.relay_parent,
                            std::nullopt,
                            network::CollationVersion::V1);
              },
              [&](auto &&) {
                SL_WARN(logger_, "Unexpected V1 collation message from.");
              });
        },
        [&](kagome::network::vstaging::CollationMessage0 &&collation_msg) {
          if (auto m =
                  if_type<network::vstaging::CollationMessage>(collation_msg)) {
            visit_in_place(
                std::move(m->get()),
                [&](kagome::network::vstaging::CollatorProtocolMessageDeclare
                        &&collation_decl) {
                  onDeclare(peer_id,
                            collation_decl.collator_id,
                            collation_decl.para_id,
                            collation_decl.signature);
                },
                [&](kagome::network::vstaging::
                        CollatorProtocolMessageAdvertiseCollation
                            &&collation_adv) {
                  onAdvertise(
                      peer_id,
                      collation_adv.relay_parent,
                      std::make_pair(collation_adv.candidate_hash,
                                     collation_adv.parent_head_data_hash),
                      network::CollationVersion::VStaging);
                },
                [&](auto &&) {
                  SL_WARN(logger_,
                          "Unexpected VStaging collation message from.");
                });
          } else {
            SL_WARN(logger_,
                    "Unexpected VStaging collation protocol message from.");
          }
        },
        [&](auto &&) {
          SL_WARN(logger_, "Unexpected versioned collation message from.");
        });
  }

  void ParachainObserverImpl::onIncomingMessage(
      const libp2p::peer::PeerId &peer_id,
      network::VersionedValidatorProtocolMessage &&message) {
    processor_->onValidationProtocolMsg(peer_id, message);
    approval_distribution_->onValidationProtocolMsg(peer_id, message);
  }

  outcome::result<network::ResponsePov> ParachainObserverImpl::OnPovRequest(
      network::RequestPov request) {
    // NOLINTNEXTLINE(hicpp-move-const-arg,performance-move-const-arg)
    return parachain_storage_->getPov(std::move(request));
  }

  outcome::result<network::CollationFetchingResponse>
  ParachainObserverImpl::OnCollationRequest(
      network::CollationFetchingRequest request) {
    /// Need to decrease rank of the peer and return error.
    return network::ProtocolError::PROTOCOL_NOT_IMPLEMENTED;
  }

  outcome::result<network::vstaging::CollationFetchingResponse>
  ParachainObserverImpl::OnCollationRequest(
      network::vstaging::CollationFetchingRequest request) {
    /// Need to decrease rank of the peer and return error.
    return network::ProtocolError::PROTOCOL_NOT_IMPLEMENTED;
  }

  void ParachainObserverImpl::onAdvertise(
      const libp2p::peer::PeerId &peer_id,
      primitives::BlockHash relay_parent,
      std::optional<std::pair<CandidateHash, Hash>> &&prospective_candidate,
      network::CollationVersion collator_protocol_version) {
    processor_->handle_advertisement(
        relay_parent, peer_id, std::move(prospective_candidate));
  }

  void ParachainObserverImpl::onDeclare(const libp2p::peer::PeerId &peer_id,
                                        network::CollatorPublicKey pubkey,
                                        network::ParachainId para_id,
                                        network::Signature signature) {
    const auto is_collating_opt = pm_->isCollating(peer_id);
    if (not is_collating_opt.has_value()) {
      logger_->warn("Received collation declaration from unknown peer {}:{}",
                    peer_id,
                    para_id);
      return;
    }

    if (is_collating_opt.value()) {
      logger_->warn("Peer is in collating state {}:{}", peer_id, para_id);
      // TODO(iceseer): https://github.com/soramitsu/kagome/issues/1513 check
      // that peer is not in collating state, or is in collating state with
      // similar pubkey and parachain id.
    }

    auto payload{peer_id.toVector()};  /// Copy because verify works with
                                       /// non-constant value.
    payload.insert(payload.end(), {'C', 'O', 'L', 'L'});

    if (auto result = crypto_provider_->verify(
            signature, std::span<uint8_t>(payload), pubkey);
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
