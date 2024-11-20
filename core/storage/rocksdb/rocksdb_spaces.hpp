/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/spaces.hpp"

#include <optional>
#include <string>

namespace kagome::storage {

  /**
   * Get a space name as a string
   */
  std::string spaceName(Space space);

  /**
   * Get the space by its name
   */
  std::optional<Space> spaceByName(std::string_view space);

}  // namespace kagome::storage
