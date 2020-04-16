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

#ifndef KAGOME_CORE_RUNTIME_METADATA_HPP
#define KAGOME_CORE_RUNTIME_METADATA_HPP

#include <outcome/outcome.hpp>
#include "primitives/opaque_metadata.hpp"

namespace kagome::runtime {

  class Metadata {
   protected:
    using OpaqueMetadata = primitives::OpaqueMetadata;

   public:
    virtual ~Metadata() = default;

    /**
     * @brief calls metadata method of Metadata runtime api
     * @return opaque metadata object or error
     */
    virtual outcome::result<OpaqueMetadata> metadata() = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_METADATA_HPP
