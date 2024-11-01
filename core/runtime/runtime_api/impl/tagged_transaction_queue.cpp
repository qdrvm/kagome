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

#include <sqlite_modern_cpp.h>

#include "blockchain/block_tree.hpp"
#include "runtime/executor.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/runtime_code_provider.hpp"

namespace kagome::runtime {

  TaggedTransactionQueueImpl::TaggedTransactionQueueImpl(
      std::shared_ptr<Executor> executor,
      LazySPtr<blockchain::BlockTree> block_tree,
      std::shared_ptr<RuntimeCodeProvider> cd_prvdr,
      std::shared_ptr<ModuleFactory> mdl_fctr)
      : executor_{std::move(executor)},
        block_tree_(block_tree),
        cd_prvdr_{cd_prvdr},
        mdl_fctr_{mdl_fctr},
        logger_{log::createLogger("TaggedTransactionQueue", "runtime")} {
    BOOST_ASSERT(executor_);
  }

  struct Extrin {
    std::string name;
    common::Buffer data;
    primitives::BlockHash root;
  };

  std::optional<std::string> read_env(const char *env_var_name) {
    const auto *path = std::getenv(env_var_name);
    if (path != nullptr) {
      return std::string(path);
    }
    return std::nullopt;
  }

  std::vector<Extrin> read_extrinsics(const std::string db_path) {
    sqlite::database db(db_path);
    std::vector<Extrin> exts;
    db << "SELECT name,ext,root FROM exts ;" >>
        [&](std::string name, std::string ext, std::string root) {
          //          if (name != "2715.txt") {
          //            return;
          //          }
          for (int i = 0; i < 1000; i++) {
            exts.emplace_back(
                Extrin{.name = name,
                       .data = common::Buffer::fromHex(ext).value(),
                       .root = primitives::BlockHash::fromHex(root).value()});
          }
          //          exts.emplace_back(
          //              Extrin{.name = std::move(name),
          //                     .data =
          //                     std::move(common::Buffer::fromHex(ext).value()),
          //                     .root =
          //                     primitives::BlockHash::fromHex(root).value()});
        };
    return exts;
  }

  void dump(const primitives::Extrinsic &ext,
            const primitives::BlockHash &hash,
            const std::string &reason) {
    constexpr auto kDumpDir = "/chain-data/dump";

    auto kFinPath = std::filesystem::path(kDumpDir) / "fin.txt";
    static bool fin_updated = false;

    using dit = std::filesystem::directory_iterator;
    const size_t existing_files = std::distance(dit(kDumpDir), dit{});
    bool fin_exists = std::filesystem::exists(kFinPath);
    size_t dump_files = existing_files;
    if (fin_exists) {
      dump_files -= 1;
    }

    if (!fin_updated) {
      fin_updated = true;

      if (dump_files) {
        std::ofstream marker;
        marker.open(kFinPath, std::ios::app);
        marker << "Node started. The last dump file of previous launch is "
               << dump_files << ".txt\n";
        marker.close();
      }
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
      // using dit = std::filesystem::directory_iterator;
      // const size_t existing_files = std::distance(dit(kDumpDir), dit{});
      next_file = dump_files;
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
    static bool first_validation_happened = false;

    auto block = block_tree_.get()->bestBlock();

    if (!first_validation_happened) {
      std::cout << "VALIDATE VALIDATE VALIDATE VALIDATE" << std::endl;
      first_validation_happened = true;
      auto db_path = read_env("EXTS_DB_PATH");
      if (db_path) {
        std::cout << "Run all extrinsics from: " << db_path.value()
                  << std::endl;
        auto exts = read_extrinsics(db_path.value());

        const auto kSourceExternal = primitives::TransactionSource::External;
        int i = 0;

        for (auto &e : exts) {
          i += 1;
          std::cout << "iter " << i << std::endl;
          primitives::Extrinsic next_ext{.data = std::move(e.data)};
          //          if (e.name == "4092.txt") {
          //            std::cout << "Break" << std::endl;
          //          }
          std::cout << "Validating: " << e.name << std::endl;

          OUTCOME_TRY(header, block_tree_.get()->getBlockHeader(e.root));
          std::string_view filename =
              "/tmp/kagome/compiled_adhoc_code_for_1000_ext_test";
          if (!std::filesystem::exists(filename)) {
            OUTCOME_TRY(code, cd_prvdr_->getCodeAt(header.state_root));
            OUTCOME_TRY(mdl_fctr_->compile(filename, *code));
          }
          OUTCOME_TRY(module, mdl_fctr_->loadCompiled(filename));
          OUTCOME_TRY(instance, module->instantiate());
          auto loop_ctx_res =
              executor_->ctx().ephemeral(instance, header.state_root);
          if (loop_ctx_res) {
            auto &loop_ctx = loop_ctx_res.value();
            try {
              auto loop_result =
                  executor_->call<primitives::TransactionValidity>(
                      loop_ctx,
                      "TaggedTransactionQueue_validate_transaction",
                      kSourceExternal,
                      next_ext,
                      e.root);
              if (loop_result.has_error()) {
                std::cout << "Error - " << loop_result.error().message()
                          << std::endl;
                auto reset_env = loop_ctx.module_instance->resetEnvironment();
                std::cout << "Reset env: " << (reset_env.has_error() == false)
                          << std::endl;
                auto reset_mem = loop_ctx.module_instance->resetMemory();
                std::cout << "Reset mem: " << (reset_mem.has_error() == false)
                          << std::endl;
              }
            } catch (...) {
              std::cout << "Validation exception" << std::endl;
            }
          } else {
            std::cout << "Failed to get root state - " << e.root.toHex()
                      << std::endl;
            continue;
            //            std::cout << "Trying current best - " <<
            //            block.hash.toHex()
            //                      << std::endl;
            //            auto loop_ctx_best_res =
            //            executor_->ctx().ephemeralAt(block.hash); if
            //            (loop_ctx_best_res) {
            //              auto &loop_ctx_best = loop_ctx_res.value();
            //              try {
            //                auto loop_result =
            //                    executor_->call<primitives::TransactionValidity>(
            //                        loop_ctx_best,
            //                        "TaggedTransactionQueue_validate_transaction",
            //                        kSourceExternal,
            //                        next_ext,
            //                        e.root);
            //                if (loop_result.has_error()) {
            //                }
            //              } catch (...) {
            //                std::cout << "Validation exception" << std::endl;
            //              }
            //            }
          }
        }  // for

      }  // db_path
    }  // first_validation

    SL_TRACE(logger_, "Validate transaction called at block {}", block);
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block.hash));
    try {
      auto result = executor_->call<primitives::TransactionValidity>(
          ctx,
          "TaggedTransactionQueue_validate_transaction",
          source,
          ext,
          block.hash);
      if (result.has_error()) {
        return std::move(result).as_failure();
      }
      return TransactionValidityAt{block, std::move(result).value()};
    } catch (...) {
      return TransactionValidityAt{block, {}};
    }
  }

}  // namespace kagome::runtime
