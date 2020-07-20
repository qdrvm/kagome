/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/authority/impl/authority_manager_impl.hpp"

#include "common/visitor.hpp"
#include "consensus/authority/authority_manager_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::authority,
                            AuthorityUpdateObserverError,
                            e) {
  using E = kagome::authority::AuthorityUpdateObserverError;
  switch (e) {
    case E::UNSUPPORTED_MESSAGE_TYPE:
      return "unsupported message type";
    default:
      return "unknown error";
  }
}

namespace kagome::authority {

  outcome::result<primitives::AuthorityList> AuthorityManagerImpl::authorities(
      const primitives::BlockInfo &block) {
    primitives::AuthorityList authorities{};
    return std::move(authorities);
  }

  outcome::result<void> AuthorityManagerImpl::onScheduledChange(
      const primitives::BlockInfo &block,
      const primitives::AuthorityList &authorities,
      primitives::BlockNumber activateAt) {
    return AuthorityManagerError::FEATURE_NOT_IMPLEMENTED_YET;
    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::onForcedChange(
      const primitives::BlockInfo &block,
      const primitives::AuthorityList &authorities,
      primitives::BlockNumber activateAt) {
    return AuthorityManagerError::FEATURE_NOT_IMPLEMENTED_YET;
    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::onOnDisabled(
      const primitives::BlockInfo &block, uint64_t authority_index) {
    return AuthorityManagerError::FEATURE_NOT_IMPLEMENTED_YET;
    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::onPause(
      const primitives::BlockInfo &block, primitives::BlockNumber activateAt) {
    return AuthorityManagerError::FEATURE_NOT_IMPLEMENTED_YET;
    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::onResume(
      const primitives::BlockInfo &block, primitives::BlockNumber activateAt) {
    return AuthorityManagerError::FEATURE_NOT_IMPLEMENTED_YET;
    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::onConsensus(
      const primitives::ConsensusEngineId &engine_id,
      const primitives::BlockInfo &block,
      const primitives::Consensus &message) {
    if (std::find(known_engines.begin(), known_engines.end(), engine_id)
        == known_engines.end()) {
      return AuthorityManagerError::UNKNOWN_ENGINE_ID;
    }

    return visit_in_place(
        message.payload,
        [this, &block](
            const primitives::ScheduledChange &msg) -> outcome::result<void> {
          return onScheduledChange(
              block, msg.authorities, block.block_number + msg.subchain_lenght);
        },
        [this, &block](const primitives::ForcedChange &msg) {
          return onForcedChange(
              block, msg.authorities, block.block_number + msg.subchain_lenght);
        },
        [this, &block](const primitives::OnDisabled &msg) {
          return onOnDisabled(block, msg.authority_index);
        },
        [this, &block](const primitives::Pause &msg) {
          return onPause(block, block.block_number + msg.subchain_lenght);
        },
        [this, &block](const primitives::Resume &msg) {
          return onResume(block, block.block_number + msg.subchain_lenght);
        },
        [](auto &) {
          return AuthorityUpdateObserverError::UNSUPPORTED_MESSAGE_TYPE;
        });
  }

}  // namespace kagome::authority
