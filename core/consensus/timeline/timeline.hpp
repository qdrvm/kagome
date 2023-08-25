/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace kagome::consensus {

  class Timeline {
   public:
    Timeline() = default;
    virtual ~Timeline() = default;

    Timeline(Timeline &&) noexcept = delete;
    Timeline(const Timeline &) = delete;
    Timeline &operator=(Timeline &&) noexcept = delete;
    Timeline &operator=(const Timeline &) = delete;
  };

}  // namespace kagome::consensus
