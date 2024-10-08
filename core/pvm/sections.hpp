/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "pvm/cursor.hpp"
#include "pvm/types.hpp"

namespace kagome::pvm {

  struct ProgramBlob {
    struct MemoryConfig {
      RO_DataSize ro_data_size;
      RW_DataSize rw_data_size;
      StackSize stack_size;
    };

    /// @brief Contains the whole program data, which is represented as spans in
    /// sections.
    std::vector<uint8_t> program_data;

    Opt<MemoryConfig> memory_config;

    Opt<RO_DataSection> ro_data;
    Opt<RW_DataSection> rw_data;
    Opt<CodeSection> code_and_jump_table;
    Opt<ImportOffsetSection> import_offsets;
    Opt<ImportsSection> import_symbols;
    Opt<ExportsSection> exports;

    Opt<DebugStringsSection> debug_strings;
    Opt<DebugLineProgramRangesSection> debug_line_program_ranges;
    Opt<DebugLineProgramsSection> debug_line_programs;

    static outcome::result<ProgramBlob> create_from(
        std::vector<uint8_t> &&program_data);

   private:
    ProgramBlob(std::vector<uint8_t> &&program_data);
    static Result<Cursor> read_section(Cursor& cursor);

    Result<void> parse_memory_config_section(Cursor &cursor);
    Result<void> parse_ro_data_section(Cursor &cursor);
    Result<void> parse_rw_data_section(Cursor &cursor);
    Result<void> parse_imports_section(Cursor &cursor);
  };

}  // namespace kagome::pvm
