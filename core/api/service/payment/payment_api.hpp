/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PAYMENT_API_HPP
#define KAGOME_PAYMENT_API_HPP

#include <boost/optional.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include "common/blob.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/runtime_dispatch_info.hpp"

namespace kagome::api {

  class PaymentApi {
   public:
    using OptionalHashRef = boost::optional<std::reference_wrapper<const common::Hash256>>;

    virtual ~PaymentApi() = default;

    virtual outcome::result<primitives::RuntimeDispatchInfo> queryInfo(
        const primitives::Extrinsic &extrinsic,
        uint32_t len,
        OptionalHashRef at)
        const = 0;
  };

}  // namespace kagome::service

#endif  // KAGOME_PAYMENT_API_HPP
