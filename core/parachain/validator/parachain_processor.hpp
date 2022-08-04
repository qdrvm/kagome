/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_PROCESSOR_HPP
#define KAGOME_PARACHAIN_PROCESSOR_HPP

#include <memory>

#include "network/protocols/req_collation_protocol.hpp"
#include "network/types/collator_messages.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {
  class PeerManager;
  class Router;
}  // namespace kagome::network

namespace kagome::crypto {
  class Sr25519Provider;
}

namespace kagome::parachain {

  struct ParachainProcessorImpl {
    ParachainProcessorImpl(
        std::shared_ptr<network::PeerManager> pm,
        std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
        std::shared_ptr<network::Router> router);
    ~ParachainProcessorImpl() = default;

    void requestCollations();

   private:
    std::shared_ptr<network::PeerManager> pm_;
    std::shared_ptr<crypto::Sr25519Provider> crypto_provider_;
    std::shared_ptr<network::Router> router_;
    log::Logger logger_ =
        log::createLogger("ParachainProcessorImpl", "parachain");
  };

}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_PROCESSOR_HPP
