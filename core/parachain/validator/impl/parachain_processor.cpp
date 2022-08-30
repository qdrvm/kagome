/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/parachain_processor.hpp"

#include <gsl/span>

#include "crypto/sr25519_provider.hpp"
#include "network/common.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/impl/protocols/protocol_error.hpp"
#include "network/peer_manager.hpp"
#include "network/router.hpp"

namespace kagome::parachain {

  ParachainProcessorImpl::ParachainProcessorImpl(
      std::shared_ptr<network::PeerManager> pm,
      std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
      std::shared_ptr<network::Router> router)
      : pm_(std::move(pm)),
        crypto_provider_(std::move(crypto_provider)),
        router_(std::move(router)) {}

  void ParachainProcessorImpl::requestCollations() {
    auto collation = pm_->pop_pending_collation();
    if (!collation) {
      logger_->info("No active collations.");
      return;
    }

    auto protocol = router_->getReqCollationProtocol();
    protocol->request(collation->peer_id,
                      network::CollationFetchingRequest{
                          .relay_parent = collation->relay_parent,
                          .para_id = collation->para_id},
                      [](outcome::result<network::CollationFetchingResponse>) {
                        /// Not implemented yet.
                      });
  }

}  // namespace kagome::parachain
