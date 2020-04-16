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

#ifndef KAGOME_CORE_PRIMITIVES_CHECK_INHERENTS_RESULT_HPP
#define KAGOME_CORE_PRIMITIVES_CHECK_INHERENTS_RESULT_HPP

#include "primitives/inherent_data.hpp"

namespace kagome::primitives {
  /**
   * @brief result of check_inherents method of BlockBuilder runtime api
   */
  struct CheckInherentsResult {
    /// Did the check succeed?
    bool is_okay = false;
    /// Did we encounter a fatal error?
    bool is_fatal_error = false;
    /// We use the `InherentData` to store our errors.
    primitives::InherentData errors;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, CheckInherentsResult &v) {
    return s >> v.is_okay >> v.is_fatal_error >> v.errors;
  }

}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_CHECK_INHERENTS_RESULT_HPP
