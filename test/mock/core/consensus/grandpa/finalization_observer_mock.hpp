/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GRANDPA_FINALIZATIONOBSERVERMOCK
#define KAGOME_GRANDPA_FINALIZATIONOBSERVERMOCK

#include "consensus/grandpa/finalization_observer.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::grandpa {

  class FinalizationObserverMock : public FinalizationObserver {
   public:
    MOCK_METHOD1(onFinalize,
                 outcome::result<void>(const primitives::BlockInfo &block));
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_GRANDPA_FINALIZATIONOBSERVERMOCK
