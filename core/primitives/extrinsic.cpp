/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/extrinsic.hpp"

using kagome::common::Buffer;

namespace kagome::primitives {

  Extrinsic::Extrinsic(Buffer data) : data_(std::move(data)) {}

  const Buffer &Extrinsic::data() const {
    return data_;
  }

}  // namespace kagome::primitives
