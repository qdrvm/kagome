/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_TYPES_NO_DATA_HPP
#define KAGOME_CORE_NETWORK_TYPES_NO_DATA_HPP

#include "primitives/common.hpp"

namespace kagome::network {

  /**
   * Is the structure without any data.
   */
  struct NoData {};
  SCALE_EMPTY_CODER(NoData);

  /**
   * @brief compares two Status instances
   * @param lhs first instance
   * @param rhs second instance
   * @return true if equal false otherwise
   */
  inline bool operator==(const NoData &lhs, const NoData &rhs) {
    return true;
  }
}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_TYPES_NO_DATA_HPP
