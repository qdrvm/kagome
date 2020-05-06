/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_SERVISE_AUTHOR_AUTHOR_API_OBSERVER_HPP
#define KAGOME_CORE_API_SERVISE_AUTHOR_AUTHOR_API_OBSERVER_HPP

#include "api/service/author/author_api.hpp"
#include "common/blob.hpp"
#include "outcome/outcome.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::api {

  class AuthorApiObserver {
   public:
    virtual ~AuthorApiObserver() = default;

    virtual outcome::result<common::Hash256> onTxMessage(
        const primitives::Extrinsic &extrinsic) = 0;
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_API_SERVISE_AUTHOR_AUTHOR_API_OBSERVER_HPP
