/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_RECOVERYMODE
#define KAGOME_APPLICATION_RECOVERYMODE

#include "application/mode.hpp"

#include <functional>

namespace kagome::application::mode {

  class RecoveryMode final : public Mode {
   public:
    RecoveryMode(std::function<int()> &&runner) : runner_(std::move(runner)) {}

    int run() const override {
      return runner_();
    }

   private:
    std::function<int()> runner_;
  };

}  // namespace kagome::application::mode

#endif  // KAGOME_APPLICATION_RECOVERYMODE
