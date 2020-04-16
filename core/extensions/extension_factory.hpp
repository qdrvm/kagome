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

#ifndef KAGOME_CORE_EXTENSIONS_EXTENSION_FACTORY_HPP
#define KAGOME_CORE_EXTENSIONS_EXTENSION_FACTORY_HPP

#include "extensions/extension.hpp"

namespace kagome::extensions {

  // Creates extension containing provided wasm memory
  class ExtensionFactory {
   public:
    virtual ~ExtensionFactory() = default;

    /**
     * Takes \param memory and creates \return extension using this memory
     */
    virtual std::shared_ptr<Extension> createExtension(
        std::shared_ptr<runtime::WasmMemory> memory) const = 0;
  };

}  // namespace kagome::extensions

#endif  // KAGOME_CORE_EXTENSIONS_EXTENSION_FACTORY_HPP
