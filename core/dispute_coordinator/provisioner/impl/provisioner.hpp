/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_PROVISIONER
#define KAGOME_DISPUTE_PROVISIONER

#include <unordered_map>

#include "dispute_coordinator/provisioner/types.hpp"
#include "dispute_coordinator/types.hpp"
#include "log/logger.hpp"

namespace kagome::dispute {

  class Provisioner final {
   public:
    virtual ~Provisioner() = default;

    void handle_RequestInherentData(const primitives::BlockHash &relay_parent);

    void handle_ProvisionableData(const primitives::BlockHash &relay_parent,
                                  ProvisionableData data);

   private:
    void on_active_leaves_update(const network::ExView &updated);

    // TODO blabla

    outcome::result<void> send_inherent_data_bg(
        PerRelayParent &per_relay_parent);

    std::unordered_map<primitives::BlockHash, PerRelayParent> per_relay_parent_;
    // InherentDelays inherent_delays_;

    log::Logger log_ = log::createLogger("Provisioner", "dispute");
  };

}  // namespace kagome::dispute
#endif  // KAGOME_DISPUTE_PROVISIONER
