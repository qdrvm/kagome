/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_EXECUTION_BENCHMARK_HPP
#define KAGOME_BLOCK_EXECUTION_BENCHMARK_HPP

#include <memory>

#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "primitives/common.hpp"

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::runtime {
  class Core;
  class ModuleRepository;
  class RuntimeCodeProvider;
}  // namespace kagome::runtime

namespace kagome::storage::trie {
  class TrieStorage;
}

namespace kagome::benchmark {

  class BlockExecutionBenchmark {
   public:
    enum class Error {
      BLOCK_WEIGHT_DECODE_FAILED,
      BLOCK_NOT_FOUND,
    };

    struct Config {
      primitives::BlockNumber start;
      primitives::BlockNumber end;
      uint16_t times;
    };

    BlockExecutionBenchmark(
        std::shared_ptr<runtime::Core> core_api,
        std::shared_ptr<const blockchain::BlockTree> block_tree,
        std::shared_ptr<runtime::ModuleRepository> module_repo,
        std::shared_ptr<const runtime::RuntimeCodeProvider> code_provider,
        std::shared_ptr<const storage::trie::TrieStorage> trie_storage);

    outcome::result<void> run(Config config);

   private:
    log::Logger logger_;
    std::shared_ptr<runtime::Core> core_api_;
    std::shared_ptr<const blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::ModuleRepository> module_repo_;
    std::shared_ptr<const runtime::RuntimeCodeProvider> code_provider_;
    std::shared_ptr<const storage::trie::TrieStorage> trie_storage_;
  };

}  // namespace kagome::benchmark

OUTCOME_HPP_DECLARE_ERROR(kagome::benchmark, BlockExecutionBenchmark::Error);

#endif  // KAGOME_BLOCK_EXECUTION_BENCHMARK_HPP
