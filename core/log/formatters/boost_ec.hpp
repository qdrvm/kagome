/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/system/error_code.hpp>

namespace boost::system {
  inline auto format_as(const error_code &ec) {
    return std::error_code{ec};
  }
}  // namespace boost::system
