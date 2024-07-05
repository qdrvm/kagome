/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/outcome/result.hpp>

namespace kagome {
  template <typename R, typename E>
  using CustomOutcome = boost::outcome_v2::
      basic_result<R, E, boost::outcome_v2::policy::default_policy<R, E, void>>;
}  // namespace kagome
