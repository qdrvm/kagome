/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_OBSERVER_DUMMY_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_OBSERVER_DUMMY_HPP

#include "common/logger.hpp"
#include "consensus/grandpa/observer.hpp"

namespace kagome::consensus::grandpa {

  class ObserverDummy : public Observer {
   public:
    ~ObserverDummy() override = default;

    ObserverDummy();

    void onPrecommit(Precommit pc) override;

    void onPrevote(Prevote pv) override;

    void onPrimaryPropose(PrimaryPropose pv) override;

   private:
    common::Logger logger_;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_OBSERVER_DUMMY_HPP
