/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_HASH_HPP
#define KAGOME_HASH_HPP

#include "common/buffer.hpp"

namespace kagome::common {

/**
 * Hash alias on const buffer. Const is used to prohibit put method which changes the content of Buffer.
 * In future will be replaced by fair implementation.
 */
using Hash = const Buffer;

}

#endif //KAGOME_HASH_HPP
