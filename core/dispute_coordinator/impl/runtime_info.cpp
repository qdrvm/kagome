/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/runtime_info.hpp"

#include "crypto/crypto_store/session_keys.hpp"
#include "dispute_coordinator/impl/errors.hpp"
#include "runtime/runtime_api/parachain_host.hpp"

namespace kagome::dispute {

  RuntimeInfo::RuntimeInfo(std::shared_ptr<runtime::ParachainHost> api,
                           std::shared_ptr<crypto::SessionKeys> session_keys)
      : api_(std::move(api)),
        session_keys_(std::move(session_keys)),
        session_index_cache_(10),
        session_info_cache_(10) {
    BOOST_ASSERT(api_ != nullptr);
    BOOST_ASSERT(session_keys_ != nullptr);
  }

  outcome::result<SessionIndex> RuntimeInfo::get_session_index_for_child(
      const primitives::BlockHash &parent) {
    auto session_index_opt = session_index_cache_.get(parent);
    if (session_index_opt.has_value()) {
      return session_index_opt.value();
    }
    OUTCOME_TRY(session_index, api_->session_index_for_child(parent));
    session_index_cache_.put(parent, session_index);
    return session_index;
  }

  outcome::result<ExtendedSessionInfo> RuntimeInfo::get_session_info(
      const primitives::BlockHash &relay_parent) {
    OUTCOME_TRY(session_index, get_session_index_for_child(relay_parent));
    OUTCOME_TRY(session_info,
                get_session_info_by_index(relay_parent, session_index));
    return std::move(session_info);
  }

  outcome::result<ExtendedSessionInfo> RuntimeInfo::get_session_info_by_index(
      const primitives::BlockHash &parent, SessionIndex session_index) {
    auto cached_session_info_opt = session_info_cache_.get(session_index);
    if (not cached_session_info_opt.has_value()) {
      OUTCOME_TRY(session_info_opt, api_->session_info(parent, session_index));

      if (not session_info_opt.has_value()) {
        return SessionObtainingError::NoSuchSession;
      }
      auto &session_info = session_info_opt.value();

      OUTCOME_TRY(validator_info, get_validator_info(session_info));

      ExtendedSessionInfo ext_session_info{session_info, validator_info};
      auto ext_session_info_ref =
          session_info_cache_.put(session_index, ext_session_info);
      return ext_session_info_ref.get();
    }
    return cached_session_info_opt.value();
  }

  outcome::result<ValidatorInfo> RuntimeInfo::get_validator_info(
      const SessionInfo &session_info) {
    auto our_index_opt = get_our_index(session_info.validators);
    if (our_index_opt.has_value()) {
      auto &our_index = our_index_opt.value();

      // Get our group index:
      auto &groups = session_info.validator_groups;
      for (GroupIndex group_index = 0; group_index < groups.size();
           ++group_index) {
        auto &group = groups[group_index];
        for (ValidatorIndex validator_index = 0; validator_index < group.size();
             ++validator_index) {
          if (our_index == validator_index) {
            return ValidatorInfo{
                .our_index = our_index,
                .our_group = group_index,
            };
          }
        }
      }

      return ValidatorInfo{
          .our_index = our_index,
          .our_group = std::nullopt,
      };
    }

    return ValidatorInfo{
        .our_index = std::nullopt,
        .our_group = std::nullopt,
    };
  }

  std::optional<ValidatorIndex> RuntimeInfo::get_our_index(
      const std::vector<ValidatorId> &validators) {
    auto indexed_key_pair_opt = session_keys_->getParaKeyPair(validators);
    if (indexed_key_pair_opt.has_value()) {
      return indexed_key_pair_opt->second;
    }
    return std::nullopt;
  }
}  // namespace kagome::dispute
