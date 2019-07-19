/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authorship/impl/block_builder_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::authorship, BlockBuilderError, e) {
  using kagome::authorship::BlockBuilderError;

  switch (e) {
    case BlockBuilderError::EXTRINSIC_APPLICATION_FAILED:
      return "extrinsic was not applied";
      break;
  }
  return "unknown error";
}
