/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_ROLLINGSESSIONWINDOW
#define KAGOME_DISPUTE_ROLLINGSESSIONWINDOW

#include "dispute_coordinator/types.hpp"
#include "network/types/collator_messages.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"

namespace kagome::dispute {

  using network::CandidateReceipt;
  using network::DisputeStatement;
  using network::SessionIndex;
  using network::ValidatorIndex;
  using network::Vote;
  using parachain::CandidateHash;
  using parachain::ValidatorSignature;
  using runtime::SessionInfo;

  class RollingSessionWindow {
   public:
    virtual ~RollingSessionWindow() = default;

    /// Access the session info for the given session index, if stored within
    /// the window.
    virtual std::optional<std::reference_wrapper<SessionInfo>> session_info(
        SessionIndex index) = 0;

    /// Access the index of the earliest session.
    virtual SessionIndex earliest_session() const = 0;

    /// Access the index of the latest session.
    virtual SessionIndex latest_session() const = 0;

    /// Returns `true` if `session_index` is contained in the window.
    virtual bool contains(SessionIndex session_index) const = 0;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_ROLLINGSESSIONWINDOW
