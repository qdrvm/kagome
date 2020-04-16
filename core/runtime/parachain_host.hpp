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

#ifndef KAGOME_CORE_RUNTIME_PARACHAIN_HOST_HPP
#define KAGOME_CORE_RUNTIME_PARACHAIN_HOST_HPP

#include "primitives/block_id.hpp"
#include "primitives/parachain_host.hpp"

namespace kagome::runtime {

  class ParachainHost {
   protected:
    using Buffer = common::Buffer;
    using ValidatorId = primitives::parachain::ValidatorId;
    using DutyRoster = primitives::parachain::DutyRoster;
    using ParachainId = primitives::parachain::ParaId;

   public:
    virtual ~ParachainHost() = default;

    /**
     * @brief Calls the ParachainHost_duty_roster function from wasm code
     * @return DutyRoster structure or error if fails
     */
    virtual outcome::result<DutyRoster> duty_roster() = 0;

    /**
     * @brief Calls the ParachainHost_active_parachains function from wasm code
     * @return vector of ParachainId items or error if fails
     */
    virtual outcome::result<std::vector<ParachainId>> active_parachains() = 0;

    /**
     * @brief Calls the ParachainHost_parachain_head function from wasm code
     * @param id parachain id
     * @return parachain head or error if fails
     */
    virtual outcome::result<boost::optional<Buffer>> parachain_head(
        ParachainId id) = 0;

    /**
     * @brief Calls the ParachainHost_parachain_code function from wasm code
     * @param id parachain id
     * @return parachain code or error if fails
     */
    virtual outcome::result<boost::optional<kagome::common::Buffer>>
    parachain_code(ParachainId id) = 0;

    /**
     * @brief reports validators list for given block_id
     * @return validators list
     */
    virtual outcome::result<std::vector<ValidatorId>> validators() = 0;
  };

}  // namespace kagome::runtime
#endif  // KAGOME_CORE_RUNTIME_PARACHAIN_HOST_HPP
