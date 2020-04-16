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

#ifndef KAGOME_APPLICATION_UTIL_HPP
#define KAGOME_APPLICATION_UTIL_HPP

#include <boost/optional.hpp>
#include "application/impl/config_reader/error.hpp"

namespace kagome::application {

  template <typename T>
  outcome::result<std::decay_t<T>> ensure(boost::optional<T> opt_entry) {
    if (not opt_entry) {
      return ConfigReaderError::MISSING_ENTRY;
    }
    return opt_entry.value();
  }

}  // namespace kagome::application

#endif  // KAGOME_APPLICATION_UTIL_HPP
