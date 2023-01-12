/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PAYMENT_API_HPP
#define KAGOME_PAYMENT_API_HPP

#include <boost/multiprecision/cpp_int.hpp>
#include <optional>

#include "common/blob.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/runtime_dispatch_info.hpp"

namespace kagome::api {

  class PaymentApi {
   public:
    using OptionalHashRef =
        std::optional<std::reference_wrapper<const common::Hash256>>;

    virtual ~PaymentApi() = default;

    virtual outcome::result<
        primitives::RuntimeDispatchInfo<primitives::OldWeight>>
    queryInfo(const primitives::Extrinsic &extrinsic,
              uint32_t len,
              OptionalHashRef at) const = 0;
  };

}  // namespace kagome::api

#endif  // KAGOME_PAYMENT_API_HPP
