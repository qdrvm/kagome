/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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
    virtual outcome::result<DutyRoster> duty_roster(
        const primitives::BlockHash &block) = 0;

    /**
     * @brief Calls the ParachainHost_active_parachains function from wasm code
     * @return vector of ParachainId items or error if fails
     */
    virtual outcome::result<std::vector<ParachainId>> active_parachains(
        const primitives::BlockHash &block) = 0;

    /**
     * @brief Calls the ParachainHost_parachain_head function from wasm code
     * @param id parachain id
     * @return parachain head or error if fails
     */
    virtual outcome::result<std::optional<Buffer>> parachain_head(
        const primitives::BlockHash &block, ParachainId id) = 0;

    /**
     * @brief Calls the ParachainHost_parachain_code function from wasm code
     * @param id parachain id
     * @return parachain code or error if fails
     */
    virtual outcome::result<std::optional<kagome::common::Buffer>>
    parachain_code(const primitives::BlockHash &block, ParachainId id) = 0;

    /**
     * @brief reports validators list for given block_id
     * @return validators list
     */
    virtual outcome::result<std::vector<ValidatorId>> validators(
        const primitives::BlockHash &block) = 0;
  };

}  // namespace kagome::runtime
#endif  // KAGOME_CORE_RUNTIME_PARACHAIN_HOST_HPP
