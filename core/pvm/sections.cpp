/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pvm/sections.hpp"
#include "pvm/cursor.hpp"

/// The magic bytes with which every program blob must start with.
static constexpr uint8_t BLOB_MAGIC[] = {'P', 'V', 'M', '\0'};
static constexpr uint8_t SECTION_MEMORY_CONFIG = 1;
static constexpr uint8_t SECTION_RO_DATA = 2;
static constexpr uint8_t SECTION_RW_DATA = 3;
static constexpr uint8_t SECTION_IMPORTS = 4;
static constexpr uint8_t SECTION_EXPORTS = 5;
static constexpr uint8_t SECTION_CODE_AND_JUMP_TABLE = 6;
static constexpr uint8_t SECTION_OPT_DEBUG_STRINGS = 128;
static constexpr uint8_t SECTION_OPT_DEBUG_LINE_PROGRAMS = 129;
static constexpr uint8_t SECTION_OPT_DEBUG_LINE_PROGRAM_RANGES = 130;
static constexpr uint8_t SECTION_END_OF_FILE = 0;
static constexpr uint8_t BLOB_VERSION_V1 = 1;
static constexpr uint8_t VERSION_DEBUG_LINE_PROGRAM_V1 = 1;

/// The maximum number of functions the program can import.
static constexpr uint32_t VM_MAXIMUM_IMPORT_COUNT = 1024;

#define CHECK_INITIALIZED(val, error) \
if (!(val)) { \
  return (error); \
} else {}

#define CHECK_NOT_INITIALIZED(val, error) CHECK_INITIALIZED(!(val), error)

namespace kagome::pvm {

  template <size_t N>
  inline bool starts_with(std::span<const uint8_t> data,
                          const uint8_t (&prefix)[N]) {
    if (data.size() >= N) {
      switch (N) {
        case sizeof(uint32_t): {
          const uint32_t *const ptr1 = (const uint32_t *const)prefix;
          const uint32_t *const ptr2 = (const uint32_t *const)&data[0];
          return (*ptr1 == *ptr2);
        }

        case sizeof(uint64_t): {
          const uint64_t *const ptr1 = (const uint64_t *const)prefix;
          const uint64_t *const ptr2 = (const uint64_t *const)&data[0];
          return (*ptr1 == *ptr2);
        }

        default: {
          for (size_t i = 0; i < N; ++i) {
            if (data[i] != prefix[i]) {
              return false;
            }
          }
          return true;
        }
      }
    }
    return false;
  }

  ProgramBlob::ProgramBlob(std::vector<uint8_t> &&p)
      : program_data(std::move(p)) {}

  outcome::result<ProgramBlob> ProgramBlob::create_from(
      std::vector<uint8_t> &&program_data) {
    ProgramBlob program(std::move(program_data));
    if (!starts_with(program.program_data, BLOB_MAGIC)) {
      return Error::MAGIC_PREFIX_MESSED;
    }

    Cursor cursor(
        Slice<uint8_t>(&program.program_data[sizeof(BLOB_MAGIC)],
                       program.program_data.size() - sizeof(BLOB_MAGIC)));

    OUTCOME_TRY(blob_version, cursor.read<uint8_t>());
    if (blob_version != BLOB_VERSION_V1) {
      return Error::UNSUPPORTED_VERSION;
    }

    while (auto section_r = cursor.read<uint8_t>()) {
      switch (section_r.value()) {
        case SECTION_END_OF_FILE:
          return program;
        case SECTION_MEMORY_CONFIG: {
          OUTCOME_TRY(program.parse_memory_config_section(cursor));
        } break;
        case SECTION_RO_DATA: {
          OUTCOME_TRY(program.parse_ro_data_section(cursor));
        } break;
        case SECTION_RW_DATA: {
          OUTCOME_TRY(program.parse_rw_data_section(cursor));
        } break;
        case SECTION_IMPORTS: {
          OUTCOME_TRY(program.parse_imports_section(cursor));
        } break;
        default: {
          OUTCOME_TRY(program.read_section(cursor));
        } break;
      }
    }

    return Error::UNEXPECTED_END_OF_FILE;
  }

  Result<Cursor> ProgramBlob::read_section(Cursor& cursor) {
    OUTCOME_TRY(section_length, cursor.read_varint());
    OUTCOME_TRY(section, cursor.read<uint8_t>(section_length));
    return Cursor(std::span<const uint8_t>(section, section_length));
  }

  Result<void> ProgramBlob::parse_imports_section(Cursor &cursor) {
    CHECK_NOT_INITIALIZED(import_offsets, Error::IMPORT_OFFSETS_SECTION_DUPLICATED);
    CHECK_NOT_INITIALIZED(import_symbols, Error::IMPORT_SYMBOLS_SECTION_DUPLICATED);

    OUTCOME_TRY(section, read_section(cursor));
    OUTCOME_TRY(import_count, section.read_varint());
    if (import_count > VM_MAXIMUM_IMPORT_COUNT) {
        return Error::TOO_MANY_IMPORTS;
    }

    const auto import_offsets_size = math::checked_mul(import_count, uint32_t(4));
    CHECK_INITIALIZED(import_offsets_size, Error::IMPORT_SECTION_CORRUPTED);

    OUTCOME_TRY(import_offsets, section.read<uint8_t>(*import_offsets_size));
    const auto import_symbols_size = section.get_len() - section.get_offset();

    OUTCOME_TRY(import_symbols, section.read<uint8_t>(import_symbols_size));
    return outcome::success();
  }

  Result<void> ProgramBlob::parse_ro_data_section(Cursor &cursor) {
    CHECK_NOT_INITIALIZED(ro_data, Error::RO_DATA_SECTION_DUPLICATED);

    OUTCOME_TRY(section, read_section(cursor));
    ro_data = section.get_section();
    return outcome::success();
  }

  Result<void> ProgramBlob::parse_rw_data_section(Cursor &cursor) {
    CHECK_NOT_INITIALIZED(rw_data, Error::RW_DATA_SECTION_DUPLICATED);

    OUTCOME_TRY(section, read_section(cursor));
    rw_data = section.get_section();
    return outcome::success();
  }

  Result<void> ProgramBlob::parse_memory_config_section(Cursor &cursor) {
    CHECK_NOT_INITIALIZED(memory_config, Error::MEMORY_CONFIG_SECTION_DUPLICATED);

    OUTCOME_TRY(section, read_section(cursor));
    OUTCOME_TRY(ro_data_size, section.read_varint());
    OUTCOME_TRY(rw_data_size, section.read_varint());
    OUTCOME_TRY(stack_size, section.read_varint());

    memory_config =  MemoryConfig {
      .ro_data_size = ro_data_size,
      .rw_data_size = rw_data_size,
      .stack_size = stack_size,
    };

    return outcome::success();
  }

}  // namespace kagome::pvm
