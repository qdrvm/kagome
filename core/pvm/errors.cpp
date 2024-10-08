/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pvm/errors.hpp"
#include "utils/stringify.hpp"

#define COMPONENT PVM
#define COMPONENT_NAME STRINGIFY(COMPONENT)

OUTCOME_CPP_DEFINE_CATEGORY(kagome::pvm, Error, e) {
  using E = kagome::pvm::Error;
  switch (e) {
    case E::NOT_IMPLEMENTED:
      return COMPONENT_NAME ": Not implemented";
    case E::MAGIC_PREFIX_MESSED:
      return COMPONENT_NAME ": No magic prefix in program data";
    case E::NOT_ENOUGH_DATA:
      return COMPONENT_NAME ": Not enough data";
    case E::UNSUPPORTED_VERSION:
      return COMPONENT_NAME ": Unsupported version";
    case E::MEMORY_CONFIG_SECTION_DUPLICATED:
      return COMPONENT_NAME ": Memory config section duplicated";
    case E::FAILED_TO_READ_UVARINT:
      return COMPONENT_NAME ": Failed to read uvarint";
    case E::MEMORY_SECTION_HAS_INCORRECT_SIZE:
      return COMPONENT_NAME ": Memory section has incorrect size";
    case E::RO_DATA_SECTION_DUPLICATED:
      return COMPONENT_NAME ": RO data section duplicated";
    case E::RW_DATA_SECTION_DUPLICATED:
      return COMPONENT_NAME ": RW data section duplicated";
    case E::IMPORT_OFFSETS_SECTION_DUPLICATED:
      return COMPONENT_NAME ": Import offsets section duplicated";
    case E::IMPORT_SYMBOLS_SECTION_DUPLICATED:
      return COMPONENT_NAME ": Import symbols section duplicated";
    case E::TOO_MANY_IMPORTS:
      return COMPONENT_NAME ": Too many imports";
    case E::UNEXPECTED_END_OF_FILE:
      return COMPONENT_NAME ": Unexpected end of file";
    case E::IMPORT_SECTION_CORRUPTED:
      return COMPONENT_NAME ": Import section corrupted";
    case E::EXPORTS_SECTION_DUPLICATED:
      return COMPONENT_NAME ": Exports section duplicated";
    case E::CODE_AND_JUMP_TABLE_SECTION_DUPLICATED:
      return COMPONENT_NAME ": Code and jump table section duplicated";
  }
  return COMPONENT_NAME ": Unknown error";
}
