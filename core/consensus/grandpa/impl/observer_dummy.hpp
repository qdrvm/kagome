/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_OBSERVER_DUMMY_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_OBSERVER_DUMMY_HPP

#include "common/logger.hpp"
#include "consensus/grandpa/observer.hpp"

namespace kagome::consensus::grandpa {

  // Will be replaced with real implementation when grandpa is merged
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
