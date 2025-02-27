/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_map>
#include "pvm/native/linux.hpp"
#include "pvm/types.hpp"

namespace kagome::pvm::sandbox {

  struct ShmAllocator {
   private:
    struct State {
      uint32_t page_shift;

      // TODO(iceseer)
      // native::linux::Mmap mmap;
      // Fd fd;
      // mutable: Mutex<GenericAllocator<Config>>,
    };
    std::shared_ptr<State> state;
  };

}  // namespace kagome::pvm::sandbox
