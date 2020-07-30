/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "finalization_composite.hpp"

namespace kagome::consensus::grandpa {

  outcome::result<void> FinalizationComposite::onFinalize(
      const primitives::BlockInfo &block) {
    for (auto &observer : observers_) {
      OUTCOME_TRY(observer->onFinalize(block));
    }
    return outcome::success();
  }

}  // namespace kagome::consensus::grandpa
