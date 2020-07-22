/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#ifndef KAGOME_GRANDPA_FINALIZATION_COMPOSITE
#define KAGOME_GRANDPA_FINALIZATION_COMPOSITE

#include "consensus/grandpa/finalization_observer.hpp"

#include "common/type_traits.hpp"

namespace kagome::consensus::grandpa {

  class FinalizationComposite final : public FinalizationObserver {
   public:
    template <class... Args,
              typename =
                  std::enable_if_t<(is_shared_ptr<Args>::value() && ...), void>>
    FinalizationComposite(Args &&... args)
        : observers_{std::forward<Args>(args)...} {};

    ~FinalizationComposite() override = default;

    /**
     * Doing something at block finalized
     * @param message
     * @return failure or nothing
     */
    void onFinalize(const primitives::BlockInfo &block) override;

   private:
    std::vector<std::shared_ptr<FinalizationObserver>> observers_;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_GRANDPA_FINALIZATION_COMPOSITE
