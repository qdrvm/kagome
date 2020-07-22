/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "finalization_composite.hpp"

namespace kagome::consensus::grandpa {

  void FinalizationComposite::onFinalize(const primitives::BlockInfo &block) {
    for (auto &observer : observers_) {
      observer->onFinalize(block);
    }
  }

}  // namespace kagome::consensus::grandpa
