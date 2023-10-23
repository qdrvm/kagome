/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"
#include "outcome/outcome.hpp"

namespace kagome::api {
  class AuthorApi;
}
namespace kagome::primitives {
  struct Extrinsic;
}

namespace kagome::network {

  class ExtrinsicObserver {
   public:
    virtual ~ExtrinsicObserver() = default;

    virtual outcome::result<common::Hash256> onTxMessage(
        const primitives::Extrinsic &extrinsic) = 0;
  };

}  // namespace kagome::network
