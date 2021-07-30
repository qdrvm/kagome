/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authorship/impl/block_builder_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::authorship, BlockBuilderError, e) {
  using E = kagome::authorship::BlockBuilderError;
  switch (e) {
    case E::EXTRINSIC_APPLICATION_FAILED:
      return "extrinsic was not applied";
    case E::EXHAUSTS_RESOURCES:
      return "extrinsic would exhaust the resources of the current block";
    case E::BAD_MANDATORY:
      return "mandatory extrinsic returned an error, block cannot be produced";
  }
  return "unknown error";
}
