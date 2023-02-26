/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKCHAIN_TREE_NODE_HPP
#define KAGOME_BLOCKCHAIN_TREE_NODE_HPP

#include <memory>
#include <unordered_set>

#include "consensus/babe/common.hpp"
#include "consensus/babe/types/epoch_digest.hpp"
#include "primitives/block_id.hpp"
#include "primitives/justification.hpp"

namespace kagome::blockchain {
  /**
   * In-memory light representation of the tree, used for efficiency and usage
   * convenience - we would only ask the database for some info, when directly
   * requested
   */
  class TreeNode : public std::enable_shared_from_this<TreeNode> {
   public:
    enum class Error { NO_CHAIN_BETWEEN_BLOCKS = 1 };

    TreeNode(const primitives::BlockHash &hash,
             primitives::BlockNumber depth,
             const std::shared_ptr<TreeNode> &parent,
             bool finalized,
             bool babe_primary);

    primitives::BlockHash block_hash;
    primitives::BlockNumber depth;
    std::weak_ptr<TreeNode> parent;
    bool finalized;
    bool babe_primary;
    bool contains_approved_para_block;

    std::vector<std::shared_ptr<TreeNode>> children{};

    /**
     * Get a node of the tree, containing block with the specified hash, if it
     * can be found
     */
    std::shared_ptr<const TreeNode> findByHash(
        const primitives::BlockHash &hash) const;

    std::shared_ptr<TreeNode> findByHash(const primitives::BlockHash &hash) {
      return std::const_pointer_cast<TreeNode>(
          std::as_const(*this).findByHash(hash));
    }

    /**
     * Exit token for applyToChain method.
     * Simply a value denoting whether applyToChain should stop.
     */
    enum class ExitToken { EXIT, CONTINUE };

    /**
     * Applies \arg op for each node starting from this and ending with \arg
     * chain_end. If op returns success(EXIT), stops.
     * @note if \arg op returns an error, execution will stop immediately and
     * the rest of nodes will not be processed
     * @note inefficient in case chain_end is not in chain with the current
     * node. Will traverse the whole subtree trying to find it.
     * @return error if chain doesn't exist or op() returns an error on a
     * call, success otherwise
     */
    outcome::result<void> applyToChain(
        const primitives::BlockInfo &chain_end,
        std::function<outcome::result<ExitToken>(TreeNode const &node)> const
            &op) const;

    primitives::BlockInfo getBlockInfo() const {
      return {depth, block_hash};
    }

    bool operator==(const TreeNode &other) const;
    bool operator!=(const TreeNode &other) const;
  };

  /**
   * Useful information about the tree & blocks it contains to make some of
   * the operations faster
   */
  struct TreeMeta {
    union WeightInfo {
      static_assert(
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
          false
#else   //__BYTE_ORDER__
          true
#endif  //__BYTE_ORDER__
          ,
          "Fix structure layout for BE");

      struct {
        uint64_t parachain_payload : 48;
        uint64_t babe_primary : 16;
      } data;
      uint64_t value;

      WeightInfo(uint64_t v) : value(v) {}
      bool operator==(WeightInfo const &r) const {
        return value == r.value;
      }
      bool operator<(WeightInfo const &r) const {
        return value < r.value;
      }
    };
    using Weight = std::pair<WeightInfo, primitives::BlockNumber>;

    explicit TreeMeta(
        const std::shared_ptr<TreeNode> &subtree_root_node,
        std::optional<primitives::Justification> last_finalized_justification);

    Weight getWeight(std::shared_ptr<TreeNode> node) const;

    /**
     * Compare node weight with best and replace if heavier.
     * @return true if heavier and replaced.
     */
    bool chooseBest(std::shared_ptr<TreeNode> node);

    std::unordered_set<primitives::BlockHash> leaves;
    std::weak_ptr<TreeNode> best_leaf;

    std::weak_ptr<TreeNode> last_finalized;
    std::optional<primitives::Justification> last_finalized_justification;
  };

  /**
   * Non-finalized part of block tree
   */
  class CachedTree {
   public:
    explicit CachedTree(std::shared_ptr<TreeNode> root,
                        std::shared_ptr<TreeMeta> metadata)
        : root_{std::move(root)}, metadata_{std::move(metadata)} {
      BOOST_ASSERT(root_ != nullptr);
      BOOST_ASSERT(metadata_ != nullptr);
    }
    /**
     * Remove nodes in block tree from current tree_ to {\arg new_trie_root}.
     * Needed to avoid cascade shared_ptr destructor calls which break
     * the stack.
     * @return new tree root
     */
    void updateTreeRoot(std::shared_ptr<TreeNode> new_trie_root,
                        primitives::Justification justification);

    void updateMeta(const std::shared_ptr<TreeNode> &new_node);

    /**
     * @brief
     * A reversal of updateMeta - it's called upon block tree branch prunung to
     * remove pruned block from leaves list, update deepest node wptr etc.
     * @param node Removed node
     */
    void removeFromMeta(const std::shared_ptr<TreeNode> &node);

    TreeNode const &getRoot() const;
    TreeNode &getRoot();

    TreeMeta const &getMetadata() const;

   private:
    std::shared_ptr<TreeNode> root_;
    std::shared_ptr<TreeMeta> metadata_;
  };
}  // namespace kagome::blockchain

OUTCOME_HPP_DECLARE_ERROR(kagome::blockchain, TreeNode::Error);

#endif  // KAGOME_BLOCKCHAIN_TREE_NODE_HPP
