/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "outcome/outcome.hpp"

namespace kagome::authorship {

  /**
   * @enum BlockBuilderError
   * @brief Represents errors that can occur while building a block.
   */
  enum class BlockBuilderError {
    /// @brief Indicates that the application of an extrinsic failed.
    EXTRINSIC_APPLICATION_FAILED = 1,
    /// @brief Indicates that the block building process has exhausted available
    /// resources.
    EXHAUSTS_RESOURCES,
    /// @brief Indicates that a mandatory extrinsic is bad, i.e., it is not
    /// valid or cannot be applied.
    BAD_MANDATORY,
  };

}  // namespace kagome::authorship

OUTCOME_HPP_DECLARE_ERROR(kagome::authorship, BlockBuilderError)
