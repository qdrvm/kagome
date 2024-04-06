/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

namespace kagome::aio {
  /**
   * Interface with destructor for `std::unique_ptr`.
   */
  class CancelDtor {
   public:
    virtual ~CancelDtor() = default;
  };

  /**
   * RAII object to cancel operation.
   */
  using Cancel = std::unique_ptr<CancelDtor>;
}  // namespace kagome::aio
