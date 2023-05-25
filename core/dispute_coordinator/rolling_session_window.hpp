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
  using network::SessionIndex;
  using network::ValidatorIndex;
  using parachain::CandidateHash;
  using parachain::ValidatorSignature;
  using runtime::SessionInfo;

  /// The session window was just advanced from one range to a new one.
  struct SessionWindowAdvanced {
    /// The previous start of the window (inclusive).
    SessionIndex prev_window_start;
    /// The previous end of the window (inclusive).
    SessionIndex prev_window_end;
    /// The new start of the window (inclusive).
    SessionIndex new_window_start;
    /// The new end of the window (inclusive).
    SessionIndex new_window_end;
  };

  /// The session window was unchanged.
  struct SessionWindowUnchanged : public Empty {};

  /// An indicated update of the rolling session window.
  using SessionWindowUpdate =
      boost::variant<SessionWindowAdvanced, SessionWindowUnchanged>;

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

    /// When inspecting a new import notification, updates the session info
    /// cache to match the session of the imported block's child.
    ///
    /// this only needs to be called on heads where we are directly notified
    /// about import, as sessions do not change often and import notifications
    /// are expected to be typically increasing in session number.
    ///
    /// some backwards drift in session index is acceptable.
    virtual outcome::result<SessionWindowUpdate> cache_session_info_for_head(
        const primitives::BlockHash &block_hash) = 0;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_ROLLINGSESSIONWINDOW
