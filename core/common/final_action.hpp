/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/final_action.hpp>

namespace kagome::common {

  template <typename F>
  using FinalAction = libp2p::common::FinalAction<F>;

  template <typename F>
  using MovableFinalAction = libp2p::common::MovableFinalAction<F>;

}  // namespace kagome::common
