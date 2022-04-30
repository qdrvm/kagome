/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/spin_lock.hpp"

namespace kagome::common {

  void spin_lock::lock() {
    while (flag_.test_and_set(std::memory_order_acquire)) {
    }
  }

  void spin_lock::unlock() {
    flag_.clear(std::memory_order_release);
  }

}  // namespace kagome::common
