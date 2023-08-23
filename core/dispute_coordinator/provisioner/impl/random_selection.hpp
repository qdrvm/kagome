/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_RANDOMSELECTION
#define KAGOME_DISPUTE_RANDOMSELECTION

#include <random>
#include "dispute_coordinator/types.hpp"
#include "log/logger.hpp"

namespace kagome::runtime {
  class ParachainHost;
}

namespace kagome::dispute {
  class DisputeCoordinator;
}

namespace kagome::dispute {

  // This module selects all RECENT disputes, fetches the votes for them from
  // dispute-coordinator and returns them as `MultiDisputeStatementSet`. If the
  // RECENT disputes are more than `kMaxDDisputesForwardedToRuntime` constant -
  // the ACTIVE disputes plus a random selection of RECENT disputes (up to
  // `kMaxDDisputesForwardedToRuntime`) are returned instead. If the ACTIVE
  // disputes are also above `kMaxDDisputesForwardedToRuntime` limit - a random
  // selection of them is generated.
  class RandomSelection final {
   public:
    /// The maximum number of disputes Provisioner will include in the inherent
    /// data.
    /// Serves as a protection not to flood the Runtime with excessive data.
    static constexpr size_t kMaxDisputesForwardedToRuntime = 1'000;

    RandomSelection(
        std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator,
        log::Logger log)
        : dispute_coordinator_(std::move(dispute_coordinator)),
          log_(std::move(log)) {}

    MultiDisputeStatementSet select_disputes();

   private:
    enum class RequestType {
      /// Query recent disputes, could be an excessive amount.
      Recent,
      /// Query the currently active and very recently concluded disputes.
      Active,
    };

    /// Request open disputes identified by `CandidateHash` and the
    /// `SessionIndex`.
    /// Returns only confirmed/concluded disputes. The rest are filtered out.
    std::vector<std::tuple<SessionIndex, CandidateHash>>
    request_confirmed_disputes(RequestType active_or_recent);

    /// Extend `acc` by `n` random, picks of not-yet-present in `acc` items of
    /// `recent` without repetition and additions of recent.
    void extend_by_random_subset_without_repetition(
        std::vector<std::tuple<SessionIndex, CandidateHash>> &acc,
        std::vector<std::tuple<SessionIndex, CandidateHash>> extension,
        size_t n);

    std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator_;
    std::default_random_engine random_;
    log::Logger log_;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_RANDOMSELECTION
