/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <span>
#include <vector>

#include "common/tagged.hpp"
#include "pvm/errors.hpp"

namespace kagome::pvm {
  /* sections types */
  using RO_DataSection =
      Tagged<std::span<const uint8_t>, struct RO_DataSectionTag>;
  using RW_DataSection =
      Tagged<std::span<const uint8_t>, struct RW_DataSectionTag>;
  using CodeSection = Tagged<std::span<const uint8_t>, struct CodeSectionTag>;
  using ImportOffsetSection =
      Tagged<std::span<const uint8_t>, struct ImportOffsetSectionTag>;
  using ImportsSection =
      Tagged<std::span<const uint8_t>, struct ImportsSectionTag>;
  using ExportsSection =
      Tagged<std::span<const uint8_t>, struct ExportsSectionTag>;
  using DebugStringsSection =
      Tagged<std::span<const uint8_t>, struct DebugStringsSectionTag>;
  using DebugLineProgramRangesSection =
      Tagged<std::span<const uint8_t>, struct DebugLineProgramRangesSectionTag>;
  using DebugLineProgramsSection =
      Tagged<std::span<const uint8_t>, struct DebugLineProgramsSectionTag>;

  /* types */
  using RO_DataSize = Tagged<uint32_t, struct RO_DataSizeTag>;
  using RW_DataSize = Tagged<uint32_t, struct RW_DataSizeTag>;
  using StackSize = Tagged<uint32_t, struct StackSizeTag>;

  template <typename T>
  using Opt = std::optional<T>;

  template <typename T>
  using Ref = std::reference_wrapper<T>;

  template <typename T>
  using CRef = std::reference_wrapper<const T>;

  template <typename T>
  using Slice = std::span<const T>;
}  // namespace kagome::pvm
