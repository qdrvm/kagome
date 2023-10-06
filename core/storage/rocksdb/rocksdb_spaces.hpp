/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/spaces.hpp"

#include <string>

namespace kagome::storage {

  /**
   * Map space item to its string name for Rocks DB needs
   * @param space - space identifier
   * @return string representation of space name
   */
  std::string spaceName(Space space);

}  // namespace kagome::storage
