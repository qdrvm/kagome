/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/beefy_justification_protocol.hpp"

#include <libp2p/common/final_action.hpp>
#include <libp2p/common/shared_fn.hpp>

#include "common/main_thread_pool.hpp"
#include "consensus/beefy/beefy.hpp"
#include "network/common.hpp"
#include "network/peer_manager.hpp"

namespace kagome::network {

  BeefyJustificationProtocol::BeefyJustificationProtocol(libp2p::Host &host,
                                                        const blockchain::GenesisBlockHash &genesis,
                                                        common::MainThreadPool &main_thread_pool,
                                                        std::shared_ptr<PeerManager> peer_manager,
                                                        std::shared_ptr<Beefy> beefy)
     : RequestResponseProtocolImpl{
           kName,
           host,
           make_protocols(kBeefyJustificationProtocol, genesis),
           log::createLogger(kName),
       },
       main_pool_handler_{main_thread_pool.handlerStarted()},
       peer_manager_{std::move(peer_manager)},
       beefy_{std::move(beefy)} {}

  std::optional<outcome::result<BeefyJustificationProtocol::ResponseType>>
  BeefyJustificationProtocol::onRxRequest(RequestType block,
                                          std::shared_ptr<Stream>) {
    OUTCOME_TRY(opt, beefy_->getJustification(block));
    if (opt) {
      return outcome::success(std::move(*opt));
    }
    return outcome::failure(ProtocolError::NO_RESPONSE);
  }

  void BeefyJustificationProtocol::fetchJustification(
      primitives::BlockNumber block) {
    REINVOKE(*main_pool_handler_, fetchJustification, block);
    if (fetching_) {
      return;
    }
    auto peer = peer_manager_->peerFinalized(block, nullptr);
    if (not peer) {
      return;
    }
    fetching_ = true;
    libp2p::common::MovableFinalAction reset_fetching{[weak{weak_from_this()}] {
      if (auto self = weak.lock()) {
        self->fetching_ = false;
      }
    }};
    doRequest(*peer,
              block,
              libp2p::SharedFn{
                  [weak_beefy{std::weak_ptr{beefy_}},
                   reset_fetching{std::move(reset_fetching)}](
                      outcome::result<consensus::beefy::BeefyJustification> r) {
                    auto beefy = weak_beefy.lock();
                    if (not beefy) {
                      return;
                    }
                    if (not r) {
                      return;
                    }
                    beefy->onMessage(std::move(r.value()));
                  }});
  }
}  // namespace kagome::network
