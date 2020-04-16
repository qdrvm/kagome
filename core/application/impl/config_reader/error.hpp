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

#ifndef KAGOME_APPLICATION_CONFIG_READER_ERROR_HPP
#define KAGOME_APPLICATION_CONFIG_READER_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::application {

  /**
   * Codes for errors that originate in configuration readers
   */
  enum class ConfigReaderError { MISSING_ENTRY = 1, PARSER_ERROR };

}

OUTCOME_HPP_DECLARE_ERROR(kagome::application, ConfigReaderError);

#endif  // KAGOME_APPLICATION_CONFIG_READER_ERROR_HPP
