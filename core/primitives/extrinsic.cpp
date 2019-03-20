/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/extrinsic.hpp"

namespace kagome::primitives {

  Extrinsic::Extrinsic(Buffer bytes) : byteArray_(std::move(bytes)) {}

  const Buffer &Extrinsic::byteArray() const {
    return byteArray_;
  }

}  // namespace kagome::primitives
