/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "justification_validator_impl.hpp"
#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus {

  JustificationValidatorImpl::JustificationValidatorImpl(
      std::shared_ptr<grandpa::Environment> grandpa_environment)
      : grandpa_environment_(std::move(grandpa_environment)) {
    BOOST_ASSERT(grandpa_environment_ != nullptr);
  }

  outcome::result<void> JustificationValidatorImpl::validateJustification(
      const primitives::BlockHash &block,
      const primitives::Justification &justification) const {

    OUTCOME_TRY(j, scale::decode<grandpa::GrandpaJustification>(justification.data));

    OUTCOME_TRY(grandpa_environment_->finalize(block, j));

    return outcome::success();
  }
}  // namespace kagome::consensus
