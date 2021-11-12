/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CODE_SUBSTITUTES_HPP
#define KAGOME_CODE_SUBSTITUTES_HPP

#include <unordered_set>

namespace kagome::primitives {

  using CodeSubstitutes = std::unordered_set<primitives::BlockHash>;

}

#endif  // KAGOME_CODE_SUBSTITUTES_HPP
