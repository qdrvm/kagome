/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/tagged_transaction_queue.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>

#include "blockchain/block_tree.hpp"
#include "runtime/executor.hpp"

namespace kagome::runtime {

  TaggedTransactionQueueImpl::TaggedTransactionQueueImpl(
      std::shared_ptr<Executor> executor,
      LazySPtr<blockchain::BlockTree> block_tree)
      : executor_{std::move(executor)},
        block_tree_(block_tree),
        logger_{log::createLogger("TaggedTransactionQueue", "runtime")} {
    BOOST_ASSERT(executor_);
  }

  void dump(const primitives::Extrinsic &ext,
            const primitives::BlockHash &hash,
            const std::string &reason) {
    constexpr auto kDumpDir = "/chain-data/dump";

    static std::optional<bool> dumps_allowed = std::nullopt;
    if (!dumps_allowed) {
      dumps_allowed = std::filesystem::is_empty(kDumpDir);
      if (!*dumps_allowed) {
        std::ofstream marker;
        marker.open(std::string(kDumpDir) + "/fin.txt", std::ios::app);
        marker << "Node started. Dumping disabled. Directory is not empty.\n";
        marker.close();
      }
    }

    if (!*dumps_allowed) {
      return;
    }

    std::ostringstream dump_data;
    dump_data << "Extrinsic hex:\n\n" << ext.data.toHex() << "\n\n";
    dump_data << "Root state:\n\n" << hash.toHex() << "\n\n";
    dump_data << "Dump reason:\n\n" << reason << "\n";

    if (!std::filesystem::exists(kDumpDir)) {
      if (!std::filesystem::create_directory(kDumpDir)) {
        std::cerr << "Cannot create dump directory: " << kDumpDir << "\n"
                  << std::flush;
        std::cerr << dump_data.str() << std::flush;
        return;
      }
    }

    static std::optional<size_t> next_file = std::nullopt;
    if (!next_file) {
      using dit = std::filesystem::directory_iterator;
      const size_t existing_files = std::distance(dit(kDumpDir), dit{});
      next_file = existing_files;
    }
    *next_file += 1;
    const auto path_and_file =
        std::string(kDumpDir) + "/" + std::to_string(*next_file) + ".txt";

    std::ofstream dump_file;
    dump_file.open(path_and_file);
    dump_file << dump_data.str();
    dump_file.close();

    if (reason == "Exception") {
      std::cout << "\n" << dump_data.str() << std::flush;
    }
  }

  outcome::result<TaggedTransactionQueue::TransactionValidityAt>
  TaggedTransactionQueueImpl::validate_transaction(
      primitives::TransactionSource source, const primitives::Extrinsic &ext) {
    auto block = block_tree_.get()->bestBlock();
    SL_TRACE(logger_, "Validate transaction called at block {}", block);
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block.hash));
    dump(ext, block.hash, "Before execution");
    try {
      auto result = executor_->call<primitives::TransactionValidity>(
          ctx,
          "TaggedTransactionQueue_validate_transaction",
          source,
          ext,
          block.hash);
      if (result.has_error()) {
        dump(ext, block.hash, "Error result: " + result.error().message());
        return std::move(result).as_failure();
      }
      return TransactionValidityAt{block, std::move(result).value()};
    } catch (...) {
      dump(ext, block.hash, "Exception");
      return TransactionValidityAt{block, {}};
    }
  }

}  // namespace kagome::runtime
