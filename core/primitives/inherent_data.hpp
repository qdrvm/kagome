/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_INHERENT_DATA_HPP
#define KAGOME_INHERENT_DATA_HPP

#include <array>
#include <map>
#include <optional>
#include <vector>

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"

namespace kagome::primitives {


  class InherentData {
   public:
    enum class Error { IDENTIFIER_ALREADY_EXISTS = 1 };

    using InherentIdentifier = std::array<uint8_t, 8>;

    /** Put data for an inherent into the internal storage.
     *
     * @arg identifier need to be unique, otherwise decoding of these
     * values will not work!
     * @arg inherent encoded data to be stored
     * @returns success if the data could be inserted an no data for an inherent
     * with the same
     */
    outcome::result<void> putData(InherentIdentifier identifier,
                                  common::Buffer inherent);

    /** Replace the data for an inherent.
     * If it does not exist, the data is just inserted.
     * @arg inherent encoded data to be stored
     */
    void replaceData(InherentIdentifier identifier, common::Buffer inherent);

    /**
     * @returns the data for the requested inherent.
     */
    outcome::result<std::optional<common::Buffer>> getData(
        const InherentIdentifier &identifier) const;

    const std::map<InherentIdentifier, common::Buffer>& getDataCollection() const;

   private:
    std::map<InherentIdentifier, common::Buffer> data_;
  };

}  // namespace kagome::primitives

OUTCOME_HPP_DECLARE_ERROR(kagome::primitives, InherentData::Error);

#endif  // KAGOME_INHERENT_DATA_HPP
