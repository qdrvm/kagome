/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/block_storage_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::blockchain, BlockStorageError, e) {
  using E = kagome::blockchain::BlockStorageError;
  switch (e) {
    case E::BLOCK_EXISTS:
      return "Block already exists on the chain";
    case E::HEADER_DOES_NOT_EXIST:
      return "Block header was not found";
    case E::BODY_DOES_NOT_EXIST:
      return "Block body was not found";
    case E::BLOCK_DATA_DOES_NOT_EXIST:
      return "Block data was not found";
    case E::JUSTIFICATION_DOES_NOT_EXIST:
      return "Justification was not found";
    case E::GENESIS_BLOCK_ALREADY_EXISTS:
      return "Genesis block already exists";
    case E::FINALIZED_BLOCK_NOT_FOUND:
      return "Finalized block not found. Possibly storage is corrupted";
    case E::BLOCK_TREE_LEAVES_NOT_FOUND:
      return "Block tree leaves not found";
    case E::GENESIS_BLOCK_NOT_FOUND:
      return "Genesis block not found";
  }
  return "Unknown error";
}
