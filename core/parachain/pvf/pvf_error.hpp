/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <qtils/enum_error_code.hpp>

namespace kagome::parachain {
  enum class PvfError : uint8_t {
    // NO_DATA conflicted with <netdb.h>
    NO_PERSISTED_DATA = 1,
    POV_SIZE,
    POV_HASH,
    CODE_HASH,
    SIGNATURE,
    HEAD_HASH,
    COMMITMENTS_HASH,
    OUTPUTS,
    PERSISTED_DATA_HASH,
    NO_CODE,
    COMPILATION_ERROR,
  };
}  // namespace kagome::parachain

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain, PvfError)
