/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COMMON_FD_LIMIT_HPP
#define KAGOME_COMMON_FD_LIMIT_HPP

#include <cstdlib>

struct rlimit;

namespace kagome::common {
  bool getFdLimit(rlimit &r);
  void setFdLimit(size_t limit);
}  // namespace kagome::common

#endif  // KAGOME_COMMON_FD_LIMIT_HPP
