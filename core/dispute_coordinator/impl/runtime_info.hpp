/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_RUNTIMEINFO
#define KAGOME_DISPUTE_RUNTIMEINFO

#include "common/lru_cache.hpp"
#include "dispute_coordinator/types.hpp"

namespace kagome::runtime {
  class ParachainHost;
}

namespace kagome::crypto {
  class SessionKeys;
}

namespace kagome::dispute {

  struct SyncCryptoStorePtr {};

  /// Information about ourselves, in case we are an `Authority`.
  ///
  /// This data is derived from the `SessionInfo` and our key as found in the
  /// keystore.
  struct ValidatorInfo {
    /// The index this very validator has in `SessionInfo` vectors, if any.
    std::optional<ValidatorIndex> our_index;
    /// The group we belong to, if any.
    std::optional<GroupIndex> our_group;

    bool operator==(const ValidatorInfo &other) const {
      return our_index == other.our_index && our_group == other.our_group;
    }
  };

  /// `SessionInfo` with additional useful data for validator nodes.
  struct ExtendedSessionInfo {
    /// Actual session info as fetched from the runtime.
    SessionInfo session_info;
    /// Contains useful information about ourselves, in case this node is a
    /// validator.
    ValidatorInfo validator_info;

    bool operator==(const ExtendedSessionInfo &other) const {
      return session_info == other.session_info
          && validator_info == other.validator_info;
    }
  };

  /// Caching of session info.
  ///
  /// It should be ensured that a cached session stays live in the cache as long
  /// as we might need it.
  class RuntimeInfo final {
   public:
    RuntimeInfo(std::shared_ptr<runtime::ParachainHost> api,
                std::shared_ptr<crypto::SessionKeys> session_keys);

    /// Returns the session index expected at any child of the `parent` block.
    /// This does not return the session index for the `parent` block.
    outcome::result<SessionIndex> get_session_index_for_child(
        const primitives::BlockHash &parent);

    /// Get `ExtendedSessionInfo` by relay parent hash.
    outcome::result<ExtendedSessionInfo> get_session_info(
        const primitives::BlockHash &relay_parent);

    /// Get `ExtendedSessionInfo` by session index.
    ///
    /// `request_session_info` still requires the parent to be passed in, so we
    /// take the parent in addition to the `SessionIndex`.
    outcome::result<ExtendedSessionInfo> get_session_info_by_index(
        const primitives::BlockHash &parent, SessionIndex session_index);

   private:
    /// Build `ValidatorInfo` for the current session.
    ///
    ///
    /// Returns: `None` if not a parachain validator.
    outcome::result<ValidatorInfo> get_validator_info(
        const SessionInfo &session_info);

    /// Get our `ValidatorIndex`.
    ///
    /// Returns: None if we are not a validator.
    std::optional<ValidatorIndex> get_our_index(
        const std::vector<ValidatorId> &validators);

    std::shared_ptr<runtime::ParachainHost> api_;

    /// Key store for determining whether we are a validator and what
    /// `ValidatorIndex` we have.
    std::shared_ptr<crypto::SessionKeys> session_keys_;

    /// Get the session index for a given relay parent.
    ///
    /// We query this up to a 100 times per block, so caching it here
    /// without roundtrips over the overseer seems sensible.
    LruCache<primitives::BlockHash, SessionIndex> session_index_cache_;

    /// Look up cached sessions by `SessionIndex`.
    LruCache<SessionIndex, ExtendedSessionInfo> session_info_cache_;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_RUNTIMEINFO
