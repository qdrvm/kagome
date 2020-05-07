/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_IMPL_EXTRINSIC_OBSERVER_IMPL_HPP
#define KAGOME_CORE_NETWORK_IMPL_EXTRINSIC_OBSERVER_IMPL_HPP

#include "network/extrinsic_observer.hpp"

#include "api/service/author/author_api.hpp"
#include "common/logger.hpp"

namespace kagome::network {

  class ExtrinsicObserverImpl : public ExtrinsicObserver {
   public:
    explicit ExtrinsicObserverImpl(std::shared_ptr<api::AuthorApi> api);
    ~ExtrinsicObserverImpl() override = default;

    outcome::result<common::Hash256> onTxMessage(
        const primitives::Extrinsic &extrinsic) override;

   private:
    std::shared_ptr<api::AuthorApi> api_;
    common::Logger logger_;
  };

}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_IMPL_EXTRINSIC_OBSERVER_IMPL_HPP
