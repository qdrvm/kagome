/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COMMON_FD_LIMIT_HPP
#define KAGOME_COMMON_FD_LIMIT_HPP

#include <cstdlib>

namespace kagome::common {
  void setFdLimit(size_t limit);
}  // namespace kagome::common

#endif  // KAGOME_COMMON_FD_LIMIT_HPP
