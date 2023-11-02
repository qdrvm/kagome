/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <span>

#include "outcome/outcome.hpp"
#include "runtime/instance_environment.hpp"
#include "storage/trie/types.hpp"

namespace kagome::runtime {

  class Module;

  class ModuleFactory {
   public:
    virtual ~ModuleFactory() = default;

    virtual outcome::result<std::shared_ptr<Module>> make(
        common::BufferView code) const = 0;
  };

}  // namespace kagome::runtime
