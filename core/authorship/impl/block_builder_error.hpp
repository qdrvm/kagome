/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_AUTHORSHIP_IMPL_BLOCK_BUILDER_ERROR_HPP
#define KAGOME_CORE_AUTHORSHIP_IMPL_BLOCK_BUILDER_ERROR_HPP

#include "outcome/outcome.hpp"

namespace kagome::authorship {

  enum class BlockBuilderError {
    EXTRINSIC_APPLICATION_FAILED = 1,
    EXHAUSTS_RESOURCES,
    BAD_MANDATORY,
  };

}

OUTCOME_HPP_DECLARE_ERROR(kagome::authorship, BlockBuilderError)

#endif  // KAGOME_CORE_AUTHORSHIP_IMPL_BLOCK_BUILDER_ERROR_HPP
