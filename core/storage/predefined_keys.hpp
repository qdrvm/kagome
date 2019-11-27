/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_PREDEFINED_KEYS_HPP
#define KAGOME_CORE_STORAGE_PREDEFINED_KEYS_HPP

#include "common/buffer.hpp"

namespace kagome::storage {

  //  const VERSION_KEY: &[u8] = b"grandpa_schema_version";
  //  const SET_STATE_KEY: &[u8] = b"grandpa_completed_round";
  //  const AUTHORITY_SET_KEY: &[u8] = b"grandpa_voters";
  //  const CONSENSUS_CHANGES_KEY: &[u8] = b"grandpa_consensus_changes";

  inline const common::Buffer kAuthoritySetKey =
      common::Buffer().put("grandpa_voters");
  inline const common::Buffer kSetStateKey =
      common::Buffer().put("grandpa_completed_round");
  ;

}  // namespace kagome::storage

#endif  // KAGOME_CORE_STORAGE_PREDEFINED_KEYS_HPP
