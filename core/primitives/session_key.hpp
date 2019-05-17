/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_SESSION_KEY_HPP
#define KAGOME_CORE_PRIMITIVES_SESSION_KEY_HPP

#include "common/blob.hpp"

namespace kagome::primitives {
  using SessionKey = common::Blob<32>;
}

#endif //KAGOME_CORE_PRIMITIVES_SESSION_KEY_HPP
