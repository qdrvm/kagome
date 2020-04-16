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

#ifndef KAGOME_BABE_HPP
#define KAGOME_BABE_HPP

#include <boost/optional.hpp>

#include "consensus/babe/common.hpp"
#include "consensus/babe/types/babe_meta.hpp"
#include "consensus/babe/types/epoch.hpp"
#include "network/babe_observer.hpp"

namespace kagome::consensus {
  /**
   * BABE protocol, used for block production in the Polkadot consensus. One of
   * the two parts in that consensus; the other is Grandpa finality
   * Read more: https://research.web3.foundation/en/latest/polkadot/BABE/Babe/
   */
  class Babe : public network::BabeObserver {
   public:
    ~Babe() override = default;

    /**
     * Creates and executes genesis epoch. Also starts event loop for new epochs
     */
    virtual void runGenesisEpoch() = 0;

    /**
     * Start a Babe production
     * @param epoch - epoch, which is going to be run
     * @param starting_slot_finish_time - when the slot, from which the BABE
     * starts, ends; for example, we start from
     * 5th slot of the some epoch. Then, we need to set time when 5th slot
     * finishes; most probably, that time will be calculated using Median
     * algorithm
     *
     * @note the function will automatically continue launching all further
     * epochs of the Babe production
     * @note in fact, it is an implementation of "Invoke-Block-Authoring" from
     * the spec
     */
    virtual void runEpoch(Epoch epoch,
                          BabeTimePoint starting_slot_finish_time) = 0;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_HPP
