/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "benchmark/block_execution_benchmark.hpp"

#include <algorithm>
#include <numeric>

#include "blockchain/block_tree.hpp"
#include "primitives/runtime_dispatch_info.hpp"
#include "runtime/runtime_api/core.hpp"
#include "storage/trie/trie_storage.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::benchmark,
                            BlockExecutionBenchmark::Error,
                            e) {
  switch (e) {
    using E = kagome::benchmark::BlockExecutionBenchmark::Error;
    case E::BLOCK_WEIGHT_DECODE_FAILED:
      return "Failed to decode block weight";
  }
  return "Unknown BlockExecutionBenchmark error";
}

#define STR_SIMPLE(a) #a
#define STR(a) STR_SIMPLE(a)
#define CONCAT_SIMPLE(a, b) a##b
#define CONCAT(a, b) CONCAT_SIMPLE(a, b)

static_assert(STR(CONCAT(2, 3)) == std::string_view("23"));

#define OUTCOME_TRY_MSG(var, expr, msg, ...)              \
  auto CONCAT(_res_, __LINE__) = (expr);                  \
  do {                                                    \
    if (CONCAT(_res_, __LINE__).has_error()) {            \
      SL_ERROR(logger_,                                   \
               "Failure on {}: {} ({})",                  \
               #expr,                                     \
               CONCAT(_res_, __LINE__).error().message(), \
               fmt::format(msg, __VA_ARGS__));            \
      return CONCAT(_res_, __LINE__).as_failure();        \
    }                                                     \
  } while (false);                                        \
  auto var = std::move(CONCAT(_res_, __LINE__).value());

#define OUTCOME_TRY_MSG_VOID(expr, msg, ...)              \
  do {                                                    \
    auto CONCAT(_res_, __LINE__) = (expr);                \
    if (CONCAT(_res_, __LINE__).has_error()) {            \
      SL_ERROR(logger_,                                   \
               "Failure on {}: {} ({})",                  \
               #expr,                                     \
               CONCAT(_res_, __LINE__).error().message(), \
               fmt::format(msg, __VA_ARGS__));            \
      return CONCAT(_res_, __LINE__).as_failure();        \
    }                                                     \
  } while (false);

namespace kagome::benchmark {

  using common::literals::operator""_hex2buf;

  BlockExecutionBenchmark::BlockExecutionBenchmark(
      std::shared_ptr<runtime::Core> core_api,
      std::shared_ptr<const blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::ModuleRepository> module_repo,
      std::shared_ptr<const runtime::RuntimeCodeProvider> code_provider,
      std::shared_ptr<const storage::trie::TrieStorage> trie_storage)
      : logger_{log::createLogger("BlockExecutionBenchmark", "benchmark")},
        core_api_{core_api},
        block_tree_{block_tree},
        module_repo_{module_repo},
        code_provider_{code_provider},
        trie_storage_{trie_storage} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(core_api_ != nullptr);
    BOOST_ASSERT(module_repo_ != nullptr);
    BOOST_ASSERT(code_provider_ != nullptr);
    BOOST_ASSERT(trie_storage_ != nullptr);
  }

  template <typename Measure>
  struct MeasureZero {
    static_assert(sizeof(Measure) == -1, "Specify zero value explicitly");
    // static const Measure zero;
  };

  template <typename Rep, typename Period>
  struct MeasureZero<std::chrono::duration<Rep, Period>> {
    static inline const std::chrono::duration<Rep, Period> zero =
        std::chrono::duration<Rep, Period>{0};
  };

  template <typename Measure>
  struct Stats {
    Stats(primitives::BlockInfo block) : block{block} {}

    Measure min() const {
      BOOST_ASSERT(!measures.empty());
      return *std::min_element(measures.begin(), measures.end());
    }

    Measure max() const {
      return *std::max_element(measures.begin(), measures.end());
    }

    Measure avg() const {
      return std::accumulate(
                 measures.begin(), measures.end(), MeasureZero<Measure>::zero)
           / measures.size();
    }

    Measure mean() const {
      return avg();
    }

    Measure median() const {
      if (!cached_median) {
        std::vector<Measure> sorted_measures(measures);
        std::sort(sorted_measures.begin(), sorted_measures.end());
        cached_median = sorted_measures[sorted_measures.size() / 2];
      }
      return *cached_median;
    }

    Measure variance() const {
      Measure square_sum;
      const auto mean_value = mean();
      for (auto &measure : measures) {
        const auto diff = measure - mean_value;
        square_sum += diff * diff;
      }
      return square_sum / measures.size();
    }

    Measure std_deviation() const {
      return std::sqrt(variance());
    }

    void add(Measure measure) {
      measures.emplace_back(std::move(measure));
      cached_median.reset();
    }

    primitives::BlockInfo getBlock() const {
      return block;
    }

   private:
    primitives::BlockInfo block;
    std::vector<Measure> measures;
    mutable std::optional<Measure> cached_median;
  };

  template <typename T>
  struct PerDispatchClass {
    SCALE_TIE(3)

    // value for `Normal` extrinsics.
    T normal;
    // value for `Operational` extrinsics.
    T operational;
    // value for `Mandatory` extrinsics.
    T mandatory;
  };

  primitives::Weight totalWeight(
      PerDispatchClass<primitives::Weight> const &weight_per_class) {
    return primitives::Weight{
        weight_per_class.normal.ref_time.number
            + weight_per_class.operational.ref_time.number
            + weight_per_class.mandatory.ref_time.number,
        weight_per_class.normal.proof_size.number
            + weight_per_class.operational.proof_size.number
            + weight_per_class.mandatory.proof_size.number};
  }

  primitives::OldWeight totalWeight(
      PerDispatchClass<primitives::OldWeight> const &weight_per_class) {
    return primitives::OldWeight{weight_per_class.normal.number
                                 + weight_per_class.operational.number
                                 + weight_per_class.mandatory.number};
  }

  using ConsumedWeight = PerDispatchClass<primitives::Weight>;

  // Hard-coded key for System::BlockWeight.
  auto BLOCK_WEIGHT_KEY =
      "26aa394eea5630e07c48ae0c9558cef734abf5cb34d6244378cddbf18e849d96"_hex2buf;

  constexpr uint64_t WEIGHT_REF_TIME_PER_NANOS = 1000;

  outcome::result<std::chrono::nanoseconds> getBlockWeightAsNanoseconds(
      storage::trie::TrieStorage const &storage,
      storage::trie::RootHash const &state) {
    OUTCOME_TRY(batch, storage.getEphemeralBatchAt(state));

    OUTCOME_TRY(enc_block_weight, batch->get(BLOCK_WEIGHT_KEY));
    scale::ScaleDecoderStream stream{enc_block_weight};
    ConsumedWeight block_weight;
    try {
      stream >> block_weight;
    } catch (std::exception &e) {
      return BlockExecutionBenchmark::Error::BLOCK_WEIGHT_DECODE_FAILED;
    }
    if (stream.hasMore(1)) {
      return BlockExecutionBenchmark::Error::BLOCK_WEIGHT_DECODE_FAILED;
    }

    return std::chrono::nanoseconds{static_cast<long long>(std::floor(
        static_cast<double>(totalWeight(block_weight).ref_time.number)
        / static_cast<double>(WEIGHT_REF_TIME_PER_NANOS)))};
  }

  outcome::result<void> BlockExecutionBenchmark::run(const Config config) {
    OUTCOME_TRY_MSG(current_hash,
                    block_tree_->getBlockHash(config.start),
                    "retrieving hash of block {}",
                    config.start);

    primitives::BlockInfo current_block_info = {config.start, current_hash};
    std::vector<primitives::BlockHash> block_hashes;
    std::vector<primitives::Block> blocks;
    while (current_block_info.number <= config.end) {
      OUTCOME_TRY_MSG(current_block_header,
                      block_tree_->getBlockHeader(current_block_info.hash),
                      "block {}",
                      current_block_info);
      OUTCOME_TRY_MSG(current_block_body,
                      block_tree_->getBlockBody(current_block_info.hash),
                      "block {}",
                      current_block_info);
      primitives::Block current_block{std::move(current_block_header),
                                      std::move(current_block_body)};
      current_block.header.digest.pop_back();
      block_hashes.emplace_back(current_block_info.hash);
      blocks.emplace_back(std::move(current_block));
      OUTCOME_TRY_MSG(next_hash,
                      block_tree_->getBlockHash(current_block_info.number + 1),
                      "retrieving hash of block {}",
                      current_block_info.number + 1);
      current_block_info.number += 1;
      current_block_info.hash = next_hash;
    }

    std::chrono::steady_clock clock;

    std::vector<Stats<std::chrono::nanoseconds>> duration_stats;
    for (size_t i = 0; i < blocks.size(); i++) {
      duration_stats.emplace_back(Stats<std::chrono::nanoseconds>{
          primitives::BlockInfo{block_hashes[i], blocks[i].header.number}});
    }
    auto duration_stat_it = duration_stats.begin();
    for (size_t block_i = 0; block_i < blocks.size(); block_i++) {
      OUTCOME_TRY(module_repo_->getInstanceAt(
          code_provider_,
          primitives::BlockInfo{block_hashes[block_i],
                                blocks[block_i].header.number},
          blocks[block_i].header));
      for (uint16_t i = 0; i < config.times; i++) {
        auto start = clock.now();
        OUTCOME_TRY_MSG_VOID(
            core_api_->execute_block(blocks[block_i], std::nullopt),
            "execution of block {}",
            block_hashes[i]);
        auto end = clock.now();
        auto duration_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        duration_stat_it->add(duration_ns);
        SL_VERBOSE(logger_,
                   "Block #{}, {} ns",
                   blocks[block_i].header.number,
                   duration_ns.count());
      }
      duration_stat_it++;
    }
    for (auto &stat : duration_stats) {
      SL_INFO(logger_,
              "Block #{}, min {} ns, avg {} ns, median {} ns, max {} ns",
              stat.getBlock().number,
              stat.min().count(),
              stat.avg().count(),
              stat.median().count(),
              stat.max().count());
      OUTCOME_TRY(
          block_weight_ns,
          getBlockWeightAsNanoseconds(
              *trie_storage_,
              blocks[stat.getBlock().number - config.start].header.state_root));
      SL_INFO(
          logger_,
          "Block {}: consumed {} ns out of declared {} ns on average. ({} %)",
          stat.getBlock().number,
          stat.avg().count(),
          block_weight_ns.count(),
          (static_cast<double>(stat.avg().count())
           / static_cast<double>(block_weight_ns.count()))
              * 100.0);
    }

    return outcome::success();
  }

}  // namespace kagome::benchmark