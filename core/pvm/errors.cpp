/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pvm/errors.hpp"
#include "utils/stringify.hpp"

#define COMPONENT_NAME "PVM"

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
    case E::DEBUG_STRINGS_SECTION_DUPLICATED:
      return COMPONENT_NAME ": Debug strings section duplicated";
    case E::DEBUG_LINE_PROGRAMS_SECTION_DUPLICATED:
      return COMPONENT_NAME ": Debug line programs section duplicated";
    case E::DEBUG_LINE_PROGRAM_RANGES_SECTION_DUPLICATED:
      return COMPONENT_NAME ": Debug line program ranges section duplicated";
    case E::UNSUPPORTED_BACKEND_KIND:
      return COMPONENT_NAME ": Unsupported backend kind";
    case E::UNSUPPORTED_SANDBOX:
      return COMPONENT_NAME ": Unsupported sandbox";
    case E::ALLOW_EXPERIMENTAL_DISABLED:
      return COMPONENT_NAME
          ": cannot enable execution cross-checking: "
          "`set_allow_experimental`/`POLKAVM_ALLOW_EXPERIMENTAL` is not "
          "enabled";
    case E::MODULE_CACHE_IS_NOT_SUPPORTED:
      return COMPONENT_NAME ": module cache is not supported yet";
    case E::SYS_CALL_FAILED:
      return COMPONENT_NAME ": syscall failed";
    case E::SYS_CALL_NOT_PERMITTED:
      return COMPONENT_NAME
          ": syscall not permitted. run 'sysctl -w "
          "vm.unprivileged_userfaultfd=1' to enable it";
    case E::LEN_UNALIGNED:
      return COMPONENT_NAME ": length is not aligned";
    case E::MEMFD_INCOMPLETE_WRITE:
      return COMPONENT_NAME ": memfd incomplete write";
  }
  return COMPONENT_NAME ": Unknown error";
}
