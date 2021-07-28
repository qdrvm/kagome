/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_UNCOMPRESS_IF_NEEDED_HPP
#define KAGOME_CORE_RUNTIME_UNCOMPRESS_IF_NEEDED_HPP

#include "common/buffer.hpp"

namespace kagome::runtime {
  void uncompressCodeIfNeeded(const common::Buffer &buf, common::Buffer &res);
}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_UNCOMPRESS_IF_NEEDED_HPP
