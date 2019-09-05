/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/block_tree_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::blockchain, BlockTreeError, e) {
  using E = kagome::blockchain::BlockTreeError;
  switch (e) {
    case E::INVALID_DB:
      return "genesis block is not provided, and the database is either empty "
             "or does not contain valid block tree";
    case E::NO_PARENT:
      return "block, which should have been added, has no known parent";
    case E::BLOCK_EXISTS:
      return "block, which should have been inserted, already exists in the "
             "tree";
    case E::HASH_FAILED:
      return "attempt to hash block part has failed";
    case E::NO_SUCH_BLOCK:
      return "block with such hash cannot be found in the local storage";
    case E::INCORRECT_ARGS:
      return "arguments, which were provided, are incorrect";
    case E::INTERNAL_ERROR:
      return "internal error happened";
  }
  return "unknown error";
}
